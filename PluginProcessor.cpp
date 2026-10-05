#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
const auto mainBusStereo = juce::AudioChannelSet::stereo();
const auto mainBusMono = juce::AudioChannelSet::mono();

float calculatePeakSidechainSample (const juce::AudioBuffer<float>& sidechain, int sampleIndex)
{
    auto peak = 0.0f;

    for (auto channel = 0; channel < sidechain.getNumChannels(); ++channel)
        peak = juce::jmax (peak, std::abs (sidechain.getReadPointer (channel)[sampleIndex]));

    return peak;
}
}

DIGuidedGateAudioProcessor::DIGuidedGateAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input", mainBusStereo, true)
                        .withOutput ("Output", mainBusStereo, true)
                        .withInput  ("Sidechain", mainBusStereo, false)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

DIGuidedGateAudioProcessor::~DIGuidedGateAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout DIGuidedGateAudioProcessor::createParameterLayout()
{
    using Parameter = std::unique_ptr<juce::RangedAudioParameter>;

    std::vector<Parameter> parameters;
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("threshold", "Threshold",
                                                                       juce::NormalisableRange<float> (-72.0f, 0.0f, 0.1f), -36.0f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("range", "Range",
                                                                       juce::NormalisableRange<float> (-80.0f, 0.0f, 0.1f), -60.0f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("attack", "Attack",
                                                                       juce::NormalisableRange<float> (0.1f, 100.0f, 0.1f, 0.4f), 2.0f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("release", "Release",
                                                                       juce::NormalisableRange<float> (5.0f, 1500.0f, 0.1f, 0.4f), 180.0f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("hysteresis", "Hysteresis",
                                                                       juce::NormalisableRange<float> (0.0f, 24.0f, 0.1f), 3.0f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("hold", "Hold",
                                                                       juce::NormalisableRange<float> (0.0f, 500.0f, 0.1f, 0.5f), 50.0f));

    return { parameters.begin(), parameters.end() };
}

const juce::String DIGuidedGateAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DIGuidedGateAudioProcessor::acceptsMidi() const { return false; }
bool DIGuidedGateAudioProcessor::producesMidi() const { return false; }
bool DIGuidedGateAudioProcessor::isMidiEffect() const { return false; }
double DIGuidedGateAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int DIGuidedGateAudioProcessor::getNumPrograms() { return 1; }
int DIGuidedGateAudioProcessor::getCurrentProgram() { return 0; }
void DIGuidedGateAudioProcessor::setCurrentProgram (int index) { juce::ignoreUnused (index); }
const juce::String DIGuidedGateAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void DIGuidedGateAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void DIGuidedGateAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    currentSampleRate = sampleRate;
    envelope = 0.0f;
    gainSmoothed = juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("range")->load());
    gateOpenState = false;
    holdSamplesRemaining = 0;
    cachedAttackMs = -1.0f;
    cachedReleaseMs = -1.0f;
    updateTimeConstants();
}

void DIGuidedGateAudioProcessor::releaseResources()
{
}

void DIGuidedGateAudioProcessor::updateTimeConstants()
{
    const auto attackMs = apvts.getRawParameterValue ("attack")->load();
    const auto releaseMs = apvts.getRawParameterValue ("release")->load();

    if (std::memcmp (&attackMs, &cachedAttackMs, sizeof (float)) == 0
        && std::memcmp (&releaseMs, &cachedReleaseMs, sizeof (float)) == 0)
        return;

    cachedAttackMs = attackMs;
    cachedReleaseMs = releaseMs;

    const auto attackSeconds = juce::jmax (0.0001f, attackMs * 0.001f);
    const auto releaseSeconds = juce::jmax (0.0001f, releaseMs * 0.001f);

    attackCoef = std::exp (-1.0f / static_cast<float> (currentSampleRate * attackSeconds));
    releaseCoef = std::exp (-1.0f / static_cast<float> (currentSampleRate * releaseSeconds));
}

bool DIGuidedGateAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainInput = layouts.getMainInputChannelSet();
    const auto mainOutput = layouts.getMainOutputChannelSet();

    if (mainInput != mainBusMono && mainInput != mainBusStereo)
        return false;

    if (mainOutput != mainInput)
        return false;

    if (layouts.inputBuses.size() > 1)
    {
        const auto sidechain = layouts.getChannelSet (true, 1);

        if (! sidechain.isDisabled() && sidechain != mainBusMono && sidechain != mainBusStereo)
            return false;
    }

    return true;
}

bool DIGuidedGateAudioProcessor::canAddBus (bool isInput) const
{
    return isInput && getBusCount (true) < 2;
}

bool DIGuidedGateAudioProcessor::canRemoveBus (bool isInput) const
{
    return isInput && getBusCount (true) > 1;
}

void DIGuidedGateAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    updateTimeConstants();

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto channel = totalNumInputChannels; channel < totalNumOutputChannels; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    if (getBusCount (true) < 2 || getChannelCountOfBus (true, 1) == 0)
        return;

    auto sidechainBuffer = getBusBuffer (buffer, true, 1);

    if (sidechainBuffer.getNumChannels() == 0)
        return;

    const auto thresholdDb = apvts.getRawParameterValue ("threshold")->load();
    const auto rangeDb = apvts.getRawParameterValue ("range")->load();
    const auto hysteresisDb = apvts.getRawParameterValue ("hysteresis")->load();
    const auto holdMs = apvts.getRawParameterValue ("hold")->load();

    const auto thresholdGain = juce::Decibels::decibelsToGain (thresholdDb);
    const auto closeThresholdGain = juce::Decibels::decibelsToGain (thresholdDb - hysteresisDb);
    const auto holdSamples = juce::roundToInt (holdMs * 0.001f * static_cast<float> (currentSampleRate));

    // Preserve the default taper slope without treating attenuation as a detector level.
    constexpr auto expansionRatio = 3.5f;

    for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto sidechainPeak = calculatePeakSidechainSample (sidechainBuffer, sample);

        if (sidechainPeak > envelope)
            envelope = sidechainPeak + attackCoef * (envelope - sidechainPeak);
        else
            envelope = sidechainPeak + releaseCoef * (envelope - sidechainPeak);

        if (gateOpenState)
        {
            if (envelope < closeThresholdGain)
            {
                if (holdSamplesRemaining > 0)
                    --holdSamplesRemaining;
                else
                    gateOpenState = false;
            }
            else
            {
                holdSamplesRemaining = holdSamples;
            }
        }
        else
        {
            if (envelope >= thresholdGain)
            {
                gateOpenState = true;
                holdSamplesRemaining = holdSamples;
            }
        }

        // Range limits the reduction; the expansion ratio sets its slope below threshold.
        float targetGain;
        if (gateOpenState)
        {
            targetGain = 1.0f;
        }
        else
        {
            const auto envelopeDb = juce::Decibels::gainToDecibels (envelope, -160.0f);
            const auto reductionDb = juce::jlimit (rangeDb, 0.0f,
                                                  (envelopeDb - thresholdDb) * (expansionRatio - 1.0f));
            targetGain = juce::Decibels::decibelsToGain (reductionDb);
        }

        const auto smoothingCoef = targetGain > gainSmoothed ? attackCoef : releaseCoef;
        gainSmoothed = targetGain + smoothingCoef * (gainSmoothed - targetGain);

        for (auto channel = 0; channel < juce::jmin (buffer.getNumChannels(), getMainBusNumOutputChannels()); ++channel)
            buffer.getWritePointer (channel)[sample] *= gainSmoothed;
    }
}

bool DIGuidedGateAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* DIGuidedGateAudioProcessor::createEditor()
{
    return new DIGuidedGateAudioProcessorEditor (*this);
}

void DIGuidedGateAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void DIGuidedGateAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DIGuidedGateAudioProcessor();
}
