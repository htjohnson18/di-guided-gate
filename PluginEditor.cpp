#include <BinaryData.h>
#include "PluginEditor.h"

DIGuidedGateAudioProcessorEditor::DIGuidedGateAudioProcessorEditor (DIGuidedGateAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      popMachineLogo (juce::ImageCache::getFromMemory (BinaryData::BUZZ_KILL_STRAIGHT_LINE_png,
                                                       BinaryData::BUZZ_KILL_STRAIGHT_LINE_pngSize)),
      thresholdAttachment (processorRef.apvts, "threshold", thresholdSlider),
      rangeAttachment (processorRef.apvts, "range", rangeSlider),
      attackAttachment (processorRef.apvts, "attack", attackSlider),
      holdAttachment (processorRef.apvts, "hold", holdSlider),
      releaseAttachment (processorRef.apvts, "release", releaseSlider),
      hysteresisAttachment (processorRef.apvts, "hysteresis", hysteresisSlider)
{
    titleLabel.setText ("DI Guided Gate", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions { 32.0f, juce::Font::bold });
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("Sidechain-driven cleanup for amp hiss, hum, and phrase gaps.", juce::dontSendNotification);
    subtitleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffc8d0d7));
    addAndMakeVisible (subtitleLabel);

    configureSlider (thresholdSlider, thresholdLabel, "Threshold");
    configureSlider (rangeSlider, rangeLabel, "Range");
    configureSlider (attackSlider, attackLabel, "Attack");
    configureSlider (holdSlider, holdLabel, "Hold");
    configureSlider (releaseSlider, releaseLabel, "Release");
    configureSlider (hysteresisSlider, hysteresisLabel, "Hysteresis");

    setSize (760, 340);
}

DIGuidedGateAudioProcessorEditor::~DIGuidedGateAudioProcessorEditor() = default;

void DIGuidedGateAudioProcessorEditor::configureSlider (juce::Slider& slider,
                                                        juce::Label& label,
                                                        const juce::String& text)
{
    const auto popMachineBlue = juce::Colour (0xff4b74d1);
    const auto popMachineRed = juce::Colour (0xffed2224);
    const auto popMachineSlate = juce::Colour (0xff253749);

    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 84, 20);
    slider.setColour (juce::Slider::rotarySliderFillColourId, popMachineBlue);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, popMachineSlate);
    slider.setColour (juce::Slider::thumbColourId, popMachineRed);
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0x33172838));
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions { 18.0f, juce::Font::bold });
    label.setColour (juce::Label::textColourId, juce::Colour (0xfff1f2f3));
    addAndMakeVisible (label);
}

void DIGuidedGateAudioProcessorEditor::drawLogoBlock (juce::Graphics& g, juce::Rectangle<int> area)
{
    auto logoArea = area.toFloat();

    if (popMachineLogo.isValid())
    {
        auto fitted = juce::RectanglePlacement (juce::RectanglePlacement::centred
                                                | juce::RectanglePlacement::onlyReduceInSize)
                          .appliedTo (juce::Rectangle<float> { 0.0f, 0.0f,
                                                               static_cast<float> (popMachineLogo.getWidth()),
                                                               static_cast<float> (popMachineLogo.getHeight()) },
                                      logoArea.reduced (0.0f, 8.0f));
        g.setOpacity (0.95f);
        g.drawImage (popMachineLogo, fitted);
    }
}

void DIGuidedGateAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient background (juce::Colour (0xff172838), 0.0f, 0.0f,
                                     juce::Colour (0xff0a1016), 0.0f, static_cast<float> (getHeight()),
                                     false);
    g.setGradientFill (background);
    g.fillAll();

    auto bounds = getLocalBounds().toFloat().reduced (14.0f);
    auto panel = bounds.reduced (8.0f);

    g.setColour (juce::Colour (0xff10191f));
    g.fillRoundedRectangle (panel, 24.0f);

    g.setColour (juce::Colour (0x40ffffff));
    g.drawRoundedRectangle (panel, 24.0f, 1.2f);

    auto content = getLocalBounds().reduced (28);
    auto header = content.removeFromTop (132);

    drawLogoBlock (g, header.removeFromRight (250));
}

void DIGuidedGateAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (34);
    auto header = bounds.removeFromTop (128);

    auto titleArea = header.removeFromLeft (bounds.getWidth() - 250);
    titleArea.removeFromTop (28);
    titleLabel.setBounds (titleArea.removeFromTop (42));
    subtitleLabel.setBounds (titleArea.removeFromTop (28));

    auto sliderArea = bounds.reduced (4, 10);
    const auto itemWidth = sliderArea.getWidth() / 6;

    auto layoutControl = [&] (juce::Slider& slider, juce::Label& label, int index)
    {
        auto area = sliderArea.withTrimmedLeft (itemWidth * index).removeFromLeft (itemWidth).reduced (8);
        label.setBounds (area.removeFromTop (30));
        slider.setBounds (area);
    };

    layoutControl (thresholdSlider, thresholdLabel, 0);
    layoutControl (rangeSlider, rangeLabel, 1);
    layoutControl (attackSlider, attackLabel, 2);
    layoutControl (holdSlider, holdLabel, 3);
    layoutControl (releaseSlider, releaseLabel, 4);
    layoutControl (hysteresisSlider, hysteresisLabel, 5);
}
