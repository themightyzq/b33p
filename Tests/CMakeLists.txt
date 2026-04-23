add_executable(B33pTests
    SanityTest.cpp
    DSP/OscillatorTests.cpp
    ${CMAKE_SOURCE_DIR}/Source/DSP/Oscillator.cpp
)

target_include_directories(B33pTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(B33pTests PRIVATE
    Catch2::Catch2WithMain
)

target_compile_features(B33pTests PRIVATE cxx_std_17)

catch_discover_tests(B33pTests)
