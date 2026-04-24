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
# Standalone GUI app target
# ---------------------------------------------------------------------------
juce_add_gui_app(B33p
    PRODUCT_NAME        "B33p"
    COMPANY_NAME        "${B33P_COMPANY_NAME}"
    BUNDLE_ID           "${B33P_BUNDLE_ID}"
    VERSION             "${PROJECT_VERSION}"
)

target_sources(B33p PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/State/B33pProcessor.cpp
    Source/State/ParameterLayout.cpp
    Source/UI/AmpEnvSection.cpp
    Source/UI/AmpEnvelopeVisualizer.cpp
    Source/UI/EffectsSection.cpp
    Source/UI/FilterSection.cpp
    Source/UI/LabeledSlider.cpp
    Source/UI/MasterSection.cpp
    Source/UI/OscillatorSection.cpp
    Source/UI/PitchEnvSection.cpp
    Source/UI/PitchEnvelopeEditor.cpp
    Source/UI/Section.cpp
    Source/DSP/Oscillator.cpp
    Source/DSP/AmpEnvelope.cpp
    Source/DSP/PitchEnvelope.cpp
    Source/DSP/LowpassFilter.cpp
    Source/DSP/Bitcrush.cpp
    Source/DSP/Distortion.cpp
    Source/DSP/Voice.cpp
)

target_include_directories(B33p PRIVATE Source)

target_compile_definitions(B33p PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:B33p,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:B33p,JUCE_VERSION>"
    B33P_VERSION_STRING="${PROJECT_VERSION}"
)

target_link_libraries(B33p
    PRIVATE
        juce::juce_audio_processors
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
