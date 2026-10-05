#include "PluginProcessor.h"
#include <cmath>
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

constexpr auto silenceDb = -160.0f;

struct GateSettings
{
    float threshold = -36.0f;
    float range = -40.0f;
    float attack = 0.1f;
    float release = 5.0f;
    float hysteresis = 3.0f;
    float hold = 0.0f;
};

// Runs at 48 kHz with 48-sample blocks, so one block is exactly one millisecond.
// The main input is constant 1.0, so the returned value is the applied gain in dB.
class GateRig
{
public:
    explicit GateRig (const GateSettings& settings)
    {
        processor.enableAllBuses();
        setParameter (processor, "threshold", settings.threshold);
        setParameter (processor, "range", settings.range);
        setParameter (processor, "attack", settings.attack);
        setParameter (processor, "release", settings.release);
        setParameter (processor, "hysteresis", settings.hysteresis);
        setParameter (processor, "hold", settings.hold);
        processor.prepareToPlay (48000.0, blockSize);
    }

    float run (float detectorDb, int milliseconds)
    {
        const auto detector = juce::Decibels::decibelsToGain (detectorDb, silenceDb);

        for (int block = 0; block < milliseconds; ++block)
        {
            fillInput (buffer, detector);
            processor.processBlock (buffer, midi);
        }

        return juce::Decibels::gainToDecibels (buffer.getSample (0, blockSize - 1), silenceDb);
    }

private:
    static constexpr int blockSize = 48;

    DIGuidedGateAudioProcessor processor;
    juce::AudioBuffer<float> buffer { 4, blockSize };
    juce::MidiBuffer midi;
};

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

void testGateOpening()
{
    // Expander taper below threshold: 2.5 dB of reduction per dB under it.
    requireNear (GateRig ({}).run (-36.5f, 100), -1.25f, 0.05f, "Detector just below Threshold must stay closed on the taper");
    requireNear (GateRig ({}).run (-35.5f, 100), 0.0f, 0.01f, "Detector just above Threshold must open to unity");
    requireNear (GateRig ({}).run (-20.0f, 100), 0.0f, 0.01f, "Loud detector must open to unity");

    GateRig rig ({});
    requireNear (rig.run (silenceDb, 100), -40.0f, 0.05f, "Silent detector must sit at the Range floor");
    requireNear (rig.run (-20.0f, 100), 0.0f, 0.01f, "Gate must open from the Range floor");
}

void testHold()
{
    GateSettings held;
    held.hold = 100.0f;

    {
        GateRig rig (held);
        rig.run (-20.0f, 50);
        requireNear (rig.run (silenceDb, 50), 0.0f, 0.1f, "Gate must stay open during Hold");
        requireNear (rig.run (silenceDb, 200), -40.0f, 0.5f, "Gate must close once Hold has expired");
    }

    {
        GateRig rig ({});
        rig.run (-20.0f, 50);
        requireNear (rig.run (silenceDb, 100), -40.0f, 0.5f, "Gate must close promptly with zero Hold");
    }

    {
        // Re-triggering while the gate is open must restart the Hold timer.
        GateRig rig (held);
        rig.run (-20.0f, 20);
        rig.run (silenceDb, 50);
        rig.run (-20.0f, 20);
        requireNear (rig.run (silenceDb, 80), 0.0f, 0.1f, "Re-triggered Hold must restart from full length");
        requireNear (rig.run (silenceDb, 200), -40.0f, 0.5f, "Gate must close after the restarted Hold");
    }
}

void testHysteresis()
{
    GateSettings wide;
    wide.hysteresis = 6.0f;

    {
        // Between the close (-42 dB) and open (-36 dB) thresholds, an open gate stays open...
        GateRig rig (wide);
        rig.run (-30.0f, 50);
        requireNear (rig.run (-39.0f, 100), 0.0f, 0.01f, "Open gate must stay open inside the hysteresis band");
        // ...and falls through it once the detector drops below the close threshold.
        requireNear (rig.run (-45.0f, 100), -22.5f, 0.1f, "Gate must close below the hysteresis band");
    }

    {
        // A closed gate must not open from inside the band.
        GateRig rig (wide);
        requireNear (rig.run (-39.0f, 100), -7.5f, 0.1f, "Closed gate must not open inside the hysteresis band");
    }

    {
        GateRig rig ({});
        rig.run (-30.0f, 50);
        requireNear (rig.run (-38.0f, 100), 0.0f, 0.01f, "Default 3 dB band must hold the gate open");
        requireNear (rig.run (-40.0f, 100), -10.0f, 0.1f, "Default 3 dB band must release below Threshold - 3 dB");
    }
}

void testNoSidechainPassthrough()
{
    const auto fillMain = [] (juce::AudioBuffer<float>& buffer)
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                buffer.setSample (channel, sample, 0.01f * static_cast<float> (sample % 50) - 0.25f + 0.1f * static_cast<float> (channel));
    };

    const auto requireUnchanged = [&] (DIGuidedGateAudioProcessor& processor, const char* message)
    {
        juce::AudioBuffer<float> buffer (2, 64), reference (2, 64);
        juce::MidiBuffer midi;
        fillMain (buffer);
        fillMain (reference);
        processor.prepareToPlay (48000.0, 64);
        processor.processBlock (buffer, midi);

        for (int channel = 0; channel < 2; ++channel)
            for (int sample = 0; sample < 64; ++sample)
                requireNear (buffer.getSample (channel, sample), reference.getSample (channel, sample), 0.0f, message);
    };

    {
        DIGuidedGateAudioProcessor processor;
        requireUnchanged (processor, "Disabled sidechain bus must pass audio through untouched");
    }

    {
        DIGuidedGateAudioProcessor processor;
        if (! processor.removeBus (true))
            throw std::runtime_error ("Sidechain bus must be removable");
        requireUnchanged (processor, "Removed sidechain bus must pass audio through untouched");
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
        testGateOpening();
        testHold();
        testHysteresis();
        testNoSidechainPassthrough();
        std::cout << "Processor regression tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
