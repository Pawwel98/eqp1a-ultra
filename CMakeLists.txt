cmake_minimum_required(VERSION 3.22)

project(PultecEQP1A VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(PultecEQP1A
    COMPANY_NAME "CervinizatorAudio"
    COMPANY_WEBSITE "https://github.com"
    COMPANY_EMAIL "info@cervinizator.audio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Crvz
    PLUGIN_CODE Peq1
    FORMATS VST3 Standalone
    PRODUCT_NAME "PULTEC EQP-1A ULTRA"
)

if(TARGET juce_vst3_helper)
    set_target_properties(juce_vst3_helper PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

target_sources(PultecEQP1A
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PultecEQP1ADSP.h
)

target_compile_definitions(PultecEQP1A
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_ENABLE_VST3_MANIFEST_GENERATOR=0
)

target_link_libraries(PultecEQP1A
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
