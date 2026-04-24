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
    ${CMAKE_SOURCE_DIR}/Source/DSP/Oscillator.cpp
    ${CMAKE_SOURCE_DIR}/Source/DSP/AmpEnvelope.cpp
    ${CMAKE_SOURCE_DIR}/Source/DSP/PitchEnvelope.cpp
    ${CMAKE_SOURCE_DIR}/Source/DSP/LowpassFilter.cpp
    ${CMAKE_SOURCE_DIR}/Source/DSP/Bitcrush.cpp
    ${CMAKE_SOURCE_DIR}/Source/DSP/Distortion.cpp
    ${CMAKE_SOURCE_DIR}/Source/DSP/Voice.cpp
    ${CMAKE_SOURCE_DIR}/Source/State/ParameterLayout.cpp
)

target_include_directories(B33pTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(B33pTests PRIVATE
    Catch2::Catch2WithMain
    juce::juce_dsp
    juce::juce_audio_processors
    juce::juce_recommended_config_flags
)

target_compile_features(B33pTests PRIVATE cxx_std_17)

catch_discover_tests(B33pTests)
