# DI-Guided Gate example plugin

cmake_minimum_required(VERSION 3.22)

project(DI_GUIDED_GATE VERSION 0.0.1)

juce_add_plugin(DIGuidedGate
    BUNDLE_ID com.tjaudio.diguidedgate
    COMPANY_NAME "TJ Audio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE TjAu
    PLUGIN_CODE DgGt
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "DI Guided Gate")

target_sources(DIGuidedGate
    PRIVATE
        PluginEditor.cpp
        PluginProcessor.cpp)

target_compile_definitions(DIGuidedGate
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(DIGuidedGate
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
