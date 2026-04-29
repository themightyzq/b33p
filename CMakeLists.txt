cmake_minimum_required(VERSION 3.22)

project(B33p
    VERSION 0.0.1
    DESCRIPTION "Sound design tool for short synthesized beeps, blips, alarms, and droid chatter"
    HOMEPAGE_URL "https://github.com/themightyzq/b33p"
    LANGUAGES CXX C
)

if(APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
endif()

set(B33P_BUNDLE_ID "com.themightyzq.b33p" CACHE STRING "Application bundle identifier")
set(B33P_COMPANY_NAME "ZQ SFX"              CACHE STRING "Company / author name")
set(B33P_JUCE_VERSION "8.0.12"              CACHE STRING "JUCE release tag to fetch")
set(B33P_CATCH2_VERSION "v3.14.0"           CACHE STRING "Catch2 release tag to fetch")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Dependencies — CPM only (see CLAUDE.md: no FetchContent, no Projucer)
# ---------------------------------------------------------------------------
include(cmake/CPM.cmake)

CPMAddPackage(
    NAME                JUCE
    GITHUB_REPOSITORY   juce-framework/JUCE
    GIT_TAG             ${B33P_JUCE_VERSION}
    VERSION             ${B33P_JUCE_VERSION}
    EXCLUDE_FROM_ALL    YES
    SYSTEM              YES
)

# ---------------------------------------------------------------------------
# Plugin + standalone target
#
# Built via juce_add_plugin so a single source set produces every shipping
# format. FORMATS values:
#   Standalone — yields b33p.app via JUCE's StandaloneFilterApp wrapper
#   VST3       — Steinberg's modern plugin format; loads in every major DAW
#   AU         — Audio Units (Apple-only; the format JUCE auto-skips on
#                non-Apple builds)
# ---------------------------------------------------------------------------
juce_add_plugin(B33p
    PRODUCT_NAME            "b33p"
    COMPANY_NAME            "${B33P_COMPANY_NAME}"
    BUNDLE_ID               "${B33P_BUNDLE_ID}"
    VERSION                 "${PROJECT_VERSION}"
    PLUGIN_MANUFACTURER_CODE Zqsf
    PLUGIN_CODE              B33p
    FORMATS                 Standalone VST3 AU
    IS_SYNTH                TRUE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    DOCUMENT_EXTENSIONS     "beep"
    ICON_BIG                "${CMAKE_SOURCE_DIR}/Source/Resources/icon_big.png"
    ICON_SMALL              "${CMAKE_SOURCE_DIR}/Source/Resources/icon_small.png"
)

target_sources(B33p PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/State/B33pProcessor.cpp
    Source/State/ParameterLayout.cpp
    Source/State/ParameterRandomizer.cpp
    Source/State/GeneratorPresets.cpp
    Source/State/PresetManager.cpp
    Source/State/ProjectFileManager.cpp
    Source/State/ProjectState.cpp
    Source/UI/AmpEnvSection.cpp
    Source/UI/AmpEnvelopeVisualizer.cpp
    Source/UI/B33pEditor.cpp
    Source/UI/EffectsSection.cpp
    Source/UI/EventOverridesDialog.cpp
    Source/UI/ExportDialog.cpp
    Source/UI/ExportTask.cpp
    Source/UI/FilterResponseVisualizer.cpp
    Source/UI/FilterSection.cpp
    Source/UI/IconButton.cpp
    Source/UI/InspectorStrip.cpp
    Source/UI/LabeledSlider.cpp
    Source/UI/MasterSection.cpp
    Source/UI/ModEffectsSection.cpp
    Source/UI/ModulationSection.cpp
    Source/UI/OscillatorSection.cpp
    Source/UI/PatternGrid.cpp
    Source/UI/PatternSection.cpp
    Source/UI/PitchEnvSection.cpp
    Source/UI/PresetBrowserDialog.cpp
    Source/UI/RandomizerWiring.cpp
    Source/UI/PitchEnvelopeEditor.cpp
    Source/UI/Section.cpp
    Source/UI/WaveformEditor.cpp
    Source/DSP/Oscillator.cpp
    Source/DSP/AmpEnvelope.cpp
    Source/DSP/PitchEnvelope.cpp
    Source/DSP/Filter.cpp
    Source/DSP/LFO.cpp
    Source/DSP/ModulationEffect.cpp
    Source/DSP/Bitcrush.cpp
    Source/DSP/Distortion.cpp
    Source/DSP/Voice.cpp
    Source/Pattern/Pattern.cpp
    Source/Pattern/PatternRenderer.cpp
    Source/Pattern/PatternSnapshot.cpp
    Source/Pattern/AudioFileWriter.cpp
)

target_include_directories(B33p PRIVATE Source)

target_compile_definitions(B33p PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:B33p,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:B33p,JUCE_VERSION>"
    B33P_VERSION_STRING="${PROJECT_VERSION}"
    # Selects the editor-bearing branch of B33pProcessor's
    # hasEditor / createEditor implementation. The tests target
    # leaves this undefined so it can link without the UI sources.
    B33P_HAS_EDITOR=1
    # b33p has never shipped a VST2, so the VST3 doesn't need to
    # claim parameter-automation continuity with one. JUCE's VST3
    # wrapper guards on this and #errors out if VST2 is absent
    # (Build_VST=0) and JUCE_VST3_CAN_REPLACE_VST2 isn't explicitly
    # zero.
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(B33p
    PRIVATE
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_gui_basics
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ---------------------------------------------------------------------------
# Tests — opt-in via BUILD_TESTING (default ON when this is the top-level
# project, OFF when B33p is consumed as a dependency).
# ---------------------------------------------------------------------------
if(PROJECT_IS_TOP_LEVEL)
    include(CTest)
    if(BUILD_TESTING)
        CPMAddPackage(
            NAME                Catch2
            GITHUB_REPOSITORY   catchorg/Catch2
            GIT_TAG             ${B33P_CATCH2_VERSION}
            EXCLUDE_FROM_ALL    YES
            SYSTEM              YES
        )

        list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
        include(Catch)

        add_subdirectory(Tests)
    endif()
endif()
