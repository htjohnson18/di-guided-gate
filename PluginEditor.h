#pragma once

#include "PluginProcessor.h"

class DIGuidedGateAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit DIGuidedGateAudioProcessorEditor (DIGuidedGateAudioProcessor&);
    ~DIGuidedGateAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    void configureSlider (juce::Slider& slider, juce::Label& label, const juce::String& text);

    DIGuidedGateAudioProcessor& processorRef;

    juce::Label titleLabel;
    juce::Label subtitleLabel;

    juce::Slider thresholdSlider;
    juce::Slider rangeSlider;
    juce::Slider attackSlider;
    juce::Slider holdSlider;
    juce::Slider releaseSlider;
    juce::Slider hysteresisSlider;

    juce::Label thresholdLabel;
    juce::Label rangeLabel;
    juce::Label attackLabel;
    juce::Label holdLabel;
    juce::Label releaseLabel;
    juce::Label hysteresisLabel;

    SliderAttachment thresholdAttachment;
    SliderAttachment rangeAttachment;
    SliderAttachment attackAttachment;
    SliderAttachment holdAttachment;
    SliderAttachment releaseAttachment;
    SliderAttachment hysteresisAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DIGuidedGateAudioProcessorEditor)
};
