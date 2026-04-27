# Shared DSP sources used by both B33pTests and the B33pRenderVoice
# listen-check CLI. Kept as a plain list (not a static library) so each
# target pulls only what it needs and link flags stay target-local.
set(B33P_DSP_SOURCES
    ${CMAKE_SOURCE_DIR}/Source/DSP/Oscillator.cpp
    ${CMAKE_SOURCE_DIR}/Source/DSP/AmpEnvelope.cpp
    ${CMAKE_SOURCE_DIR}/Source/DSP/PitchEnvelope.cpp
    ${CMAKE_SOURCE_DIR}/Source/DSP/LowpassFilter.cpp
    ${CMAKE_SOURCE_DIR}/Source/DSP/Bitcrush.cpp
    ${CMAKE_SOURCE_DIR}/Source/DSP/Distortion.cpp
    ${CMAKE_SOURCE_DIR}/Source/DSP/Voice.cpp
)

add_executable(B33pTests
    SanityTest.cpp
    DSP/OscillatorTests.cpp
    DSP/AmpEnvelopeTests.cpp
    DSP/PitchEnvelopeTests.cpp
    DSP/LowpassFilterTests.cpp
    DSP/BitcrushTests.cpp
    DSP/DistortionTests.cpp
    DSP/VoiceTests.cpp
    State/ParameterLayoutTests.cpp
    State/ParameterRandomizerTests.cpp
    State/ProjectStateTests.cpp
    Pattern/PatternTests.cpp
    Pattern/PatternSnapshotTests.cpp
    Pattern/PatternRendererTests.cpp
    Pattern/SnapMathTests.cpp
    Pattern/WavWriterTests.cpp
    Harness/VoiceRenderer.cpp
    Harness/VoiceRendererTests.cpp
    ${B33P_DSP_SOURCES}
    ${CMAKE_SOURCE_DIR}/Source/State/ParameterLayout.cpp
    ${CMAKE_SOURCE_DIR}/Source/State/ParameterRandomizer.cpp
    ${CMAKE_SOURCE_DIR}/Source/State/ProjectFileManager.cpp
    ${CMAKE_SOURCE_DIR}/Source/State/ProjectState.cpp
    ${CMAKE_SOURCE_DIR}/Source/State/B33pProcessor.cpp
    ${CMAKE_SOURCE_DIR}/Source/Pattern/Pattern.cpp
    ${CMAKE_SOURCE_DIR}/Source/Pattern/PatternRenderer.cpp
    ${CMAKE_SOURCE_DIR}/Source/Pattern/PatternSnapshot.cpp
    ${CMAKE_SOURCE_DIR}/Source/Pattern/WavWriter.cpp
)

target_include_directories(B33pTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(B33pTests PRIVATE
    Catch2::Catch2WithMain
    juce::juce_dsp
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_recommended_config_flags
)

# Match the B33p app target: skip the juce_core curl path (no libcurl
# on the minimal Linux CI image) and the juce_gui_extra WebBrowser
# path (no gtk/gtk.h either). Without these, linking juce_dsp pulls
# in the curl symbols and linking juce_audio_processors pulls in
# juce_gui_extra, both of which fail on ubuntu-latest.
target_compile_definitions(B33pTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_compile_features(B33pTests PRIVATE cxx_std_17)

catch_discover_tests(B33pTests)

# -----------------------------------------------------------------------------
# B33pRenderVoice — a listen-check CLI. Given a duration, pitch, waveform, and
# output path, it renders one Voice event to a mono 16-bit WAV so a human can
# play the file and hear whether the DSP sounds right. Not a test — never
# registered with ctest — just a developer tool built alongside the suite.
# -----------------------------------------------------------------------------
add_executable(B33pRenderVoice
    Harness/RenderVoice.cpp
    Harness/VoiceRenderer.cpp
    ${B33P_DSP_SOURCES}
)

target_include_directories(B33pRenderVoice PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(B33pRenderVoice PRIVATE
    juce::juce_dsp
    juce::juce_audio_formats
    juce::juce_recommended_config_flags
)

target_compile_definitions(B33pRenderVoice PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_compile_features(B33pRenderVoice PRIVATE cxx_std_17)
