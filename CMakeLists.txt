cmake_minimum_required(VERSION 3.22)
project(VoxMIDI VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE is pulled automatically — nothing to install by hand.
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

# An audio-in -> MIDI-out plugin: not a synth, takes audio, produces MIDI.
juce_add_plugin(VoxMIDI
    COMPANY_NAME "VoxMIDI"
    PLUGIN_MANUFACTURER_CODE Vxmi
    PLUGIN_CODE Vxm1
    FORMATS VST3 Standalone
    PRODUCT_NAME "VoxMIDI"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Analyzer)

juce_generate_juce_header(VoxMIDI)

target_sources(VoxMIDI PRIVATE
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
    source/VoxLookAndFeel.cpp
    source/dsp/YinPitchDetector.cpp
    source/dsp/NoteTracker.cpp
    source/dsp/ScaleSnapper.cpp
    source/dsp/KeyEstimator.cpp)

target_include_directories(VoxMIDI PRIVATE source)

target_compile_definitions(VoxMIDI PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(VoxMIDI
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
