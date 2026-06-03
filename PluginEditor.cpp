#include "PluginEditor.h"

DIGuidedGateAudioProcessorEditor::DIGuidedGateAudioProcessorEditor (DIGuidedGateAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      thresholdAttachment (processorRef.apvts, "threshold", thresholdSlider),
      rangeAttachment (processorRef.apvts, "range", rangeSlider),
      attackAttachment (processorRef.apvts, "attack", attackSlider),
      holdAttachment (processorRef.apvts, "hold", holdSlider),
      releaseAttachment (processorRef.apvts, "release", releaseSlider),
      hysteresisAttachment (processorRef.apvts, "hysteresis", hysteresisSlider)
{
    titleLabel.setText ("DI Guided Gate", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions { 28.0f, juce::Font::bold });
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("Use a clean DI sidechain to clamp amp noise between phrases.", juce::dontSendNotification);
    subtitleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffc9d7d0));
    addAndMakeVisible (subtitleLabel);

    configureSlider (thresholdSlider, thresholdLabel, "Threshold");
    configureSlider (rangeSlider, rangeLabel, "Range");
    configureSlider (attackSlider, attackLabel, "Attack");
    configureSlider (holdSlider, holdLabel, "Hold");
    configureSlider (releaseSlider, releaseLabel, "Release");
    configureSlider (hysteresisSlider, hysteresisLabel, "Hysteresis");

    setSize (640, 260);
}

DIGuidedGateAudioProcessorEditor::~DIGuidedGateAudioProcessorEditor() = default;

void DIGuidedGateAudioProcessorEditor::configureSlider (juce::Slider& slider,
                                                        juce::Label& label,
                                                        const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 84, 20);
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffd9a441));
    slider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffffd28a));
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0x22111111));
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (label);
}

void DIGuidedGateAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient background (juce::Colour (0xff182321), 0.0f, 0.0f,
                                     juce::Colour (0xff0a0e10), 0.0f, static_cast<float> (getHeight()),
                                     false);
    g.setGradientFill (background);
    g.fillAll();

    auto bounds = getLocalBounds().toFloat().reduced (12.0f);
    g.setColour (juce::Colour (0x30ffffff));
    g.drawRoundedRectangle (bounds, 18.0f, 1.0f);
}

void DIGuidedGateAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (20);
    auto header = bounds.removeFromTop (56);
    titleLabel.setBounds (header.removeFromTop (32));
    subtitleLabel.setBounds (header);

    auto sliderArea = bounds.reduced (0, 12);
    const auto itemWidth = sliderArea.getWidth() / 6;

    auto layoutControl = [&] (juce::Slider& slider, juce::Label& label, int index)
    {
        auto area = sliderArea.withTrimmedLeft (itemWidth * index).removeFromLeft (itemWidth).reduced (8);
        label.setBounds (area.removeFromTop (24));
        slider.setBounds (area);
    };

    layoutControl (thresholdSlider, thresholdLabel, 0);
    layoutControl (rangeSlider, rangeLabel, 1);
    layoutControl (attackSlider, attackLabel, 2);
    layoutControl (holdSlider, holdLabel, 3);
    layoutControl (releaseSlider, releaseLabel, 4);
    layoutControl (hysteresisSlider, hysteresisLabel, 5);
}
