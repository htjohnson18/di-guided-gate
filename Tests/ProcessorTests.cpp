#include "PluginProcessor.h"
#include <iostream>
#include <stdexcept>

namespace
{
void setParameter (DIGuidedGateAudioProcessor& processor, const char* id, float value)
{
    auto* parameter = processor.apvts.getParameter (id);
    parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

void requireNear (float actual, float expected, float tolerance, const char* message)
{
    if (! std::isfinite (actual) || std::abs (actual - expected) > tolerance)
        throw std::runtime_error (message);
}

void fillInput (juce::AudioBuffer<float>& buffer, float detector)
{
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        buffer.setSample (0, sample, 1.0f);
        buffer.setSample (1, sample, 1.0f);
        buffer.setSample (2, sample, detector);
        buffer.setSample (3, sample, detector);
    }
}

void testStartup()
{
    for (const auto sampleRate : { 44100.0, 48000.0, 192000.0 })
        for (const auto range : { -80.0f, -60.0f, -30.0f, 0.0f })
        {
            DIGuidedGateAudioProcessor processor;
            processor.enableAllBuses();
            setParameter (processor, "range", range);
            processor.prepareToPlay (sampleRate, 64);
            juce::AudioBuffer<float> buffer (4, 64);
            juce::MidiBuffer midi;
            fillInput (buffer, 0.0f);
            processor.processBlock (buffer, midi);
            const auto expected = juce::Decibels::decibelsToGain (range);
            for (int channel = 0; channel < 2; ++channel)
                for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                    requireNear (buffer.getSample (channel, sample), expected, 0.00001f,
                                 "Silent startup must begin at the configured floor");
        }
}

void testExpansion()
{
    for (const auto threshold : { -72.0f, -36.0f, 0.0f })
        for (const auto range : { -80.0f, -60.0f, -36.0f, -30.0f, 0.0f })
            for (const auto depth : { 0.1f, 4.0f, 20.0f, 40.0f })
            {
                DIGuidedGateAudioProcessor processor;
                processor.enableAllBuses();
                setParameter (processor, "threshold", threshold);
                setParameter (processor, "range", range);
                setParameter (processor, "attack", 0.1f);
                setParameter (processor, "release", 5.0f);
                processor.prepareToPlay (48000.0, 256);
                juce::AudioBuffer<float> buffer (4, 256);
                juce::MidiBuffer midi;
                const auto detector = juce::Decibels::decibelsToGain (threshold - depth, -160.0f);
                for (int block = 0; block < 100; ++block)
                {
                    fillInput (buffer, detector);
                    processor.processBlock (buffer, midi);
                }
                const auto expectedDb = juce::jmax (range, -depth * 2.5f);
                const auto actualDb = juce::Decibels::gainToDecibels (buffer.getSample (0, 255), -160.0f);
                requireNear (actualDb, expectedDb, 0.02f, "Expansion must be independent of Range ordering");
            }
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    try
    {
        testStartup();
        testExpansion();
        std::cout << "Processor regression tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
