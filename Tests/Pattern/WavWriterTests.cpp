#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "Pattern/WavWriter.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <cmath>
#include <memory>

using B33p::BitDepth;
using B33p::ChannelMode;
using B33p::writeWav;
using Catch::Approx;

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr int    kNumSamples = 4800;   // 0.1 s at 48 kHz

    juce::AudioBuffer<float> makeTestSine(double frequencyHz)
    {
        juce::AudioBuffer<float> b { 1, kNumSamples };
        for (int i = 0; i < kNumSamples; ++i)
        {
            const double phase = 2.0 * juce::MathConstants<double>::pi
                                       * frequencyHz * i / kSampleRate;
            // amplitude 0.5 leaves headroom so 8-bit quantisation
            // doesn't clip
            b.setSample(0, i, 0.5f * static_cast<float>(std::sin(phase)));
        }
        return b;
    }

    std::unique_ptr<juce::AudioFormatReader> openReader(const juce::File& f)
    {
        juce::AudioFormatManager mgr;
        mgr.registerBasicFormats();
        return std::unique_ptr<juce::AudioFormatReader>(mgr.createReaderFor(f));
    }
}

TEST_CASE("WavWriter: 16-bit mono round-trips within bit-depth tolerance",
          "[pattern][wav]")
{
    const auto input = makeTestSine(440.0);
    const auto tmp   = juce::File::createTempFile(".wav");

    REQUIRE(writeWav(input, kSampleRate, BitDepth::Sixteen,
                     ChannelMode::Mono, tmp));

    auto reader = openReader(tmp);
    REQUIRE(reader != nullptr);
    REQUIRE(reader->sampleRate    == kSampleRate);
    REQUIRE(reader->numChannels   == 1u);
    REQUIRE(reader->bitsPerSample == 16u);
    REQUIRE(static_cast<int>(reader->lengthInSamples) == kNumSamples);

    juce::AudioBuffer<float> readBack { 1, kNumSamples };
    reader->read(&readBack, 0, kNumSamples, 0, true, false);

    // 16-bit step ≈ 1/32768 ≈ 3e-5; tolerance well above that for
    // any rounding-direction quirks.
    for (int i = 0; i < kNumSamples; ++i)
        REQUIRE(std::fabs(readBack.getSample(0, i) - input.getSample(0, i)) < 1e-4f);

    reader.reset();
    tmp.deleteFile();
}

TEST_CASE("WavWriter: 24-bit mono round-trips at near-full precision",
          "[pattern][wav]")
{
    const auto input = makeTestSine(440.0);
    const auto tmp   = juce::File::createTempFile(".wav");

    REQUIRE(writeWav(input, kSampleRate, BitDepth::TwentyFour,
                     ChannelMode::Mono, tmp));

    auto reader = openReader(tmp);
    REQUIRE(reader != nullptr);
    REQUIRE(reader->bitsPerSample == 24u);

    juce::AudioBuffer<float> readBack { 1, kNumSamples };
    reader->read(&readBack, 0, kNumSamples, 0, true, false);

    // 24-bit step ≈ 1/8e6 ≈ 1.2e-7. Tolerance of 1e-6 is loose
    // enough for any conversion rounding.
    for (int i = 0; i < kNumSamples; ++i)
        REQUIRE(std::fabs(readBack.getSample(0, i) - input.getSample(0, i)) < 1e-6f);

    reader.reset();
    tmp.deleteFile();
}

TEST_CASE("WavWriter: 8-bit mono round-trips within coarse 8-bit tolerance",
          "[pattern][wav]")
{
    const auto input = makeTestSine(440.0);
    const auto tmp   = juce::File::createTempFile(".wav");

    REQUIRE(writeWav(input, kSampleRate, BitDepth::Eight,
                     ChannelMode::Mono, tmp));

    auto reader = openReader(tmp);
    REQUIRE(reader != nullptr);
    REQUIRE(reader->bitsPerSample == 8u);

    juce::AudioBuffer<float> readBack { 1, kNumSamples };
    reader->read(&readBack, 0, kNumSamples, 0, true, false);

    // 8-bit step ≈ 1/128 ≈ 0.008. Tolerance 0.02 to absorb
    // rounding direction differences (some platforms may use
    // unsigned offset conventions).
    for (int i = 0; i < kNumSamples; ++i)
        REQUIRE(std::fabs(readBack.getSample(0, i) - input.getSample(0, i)) < 0.02f);

    reader.reset();
    tmp.deleteFile();
}

TEST_CASE("WavWriter: stereo-duplicated writes both channels with identical data",
          "[pattern][wav]")
{
    const auto input = makeTestSine(440.0);
    const auto tmp   = juce::File::createTempFile(".wav");

    REQUIRE(writeWav(input, kSampleRate, BitDepth::Sixteen,
                     ChannelMode::StereoDuplicated, tmp));

    auto reader = openReader(tmp);
    REQUIRE(reader != nullptr);
    REQUIRE(reader->numChannels == 2u);
    REQUIRE(static_cast<int>(reader->lengthInSamples) == kNumSamples);

    juce::AudioBuffer<float> readBack { 2, kNumSamples };
    reader->read(&readBack, 0, kNumSamples, 0, true, true);

    for (int i = 0; i < kNumSamples; ++i)
    {
        const float left  = readBack.getSample(0, i);
        const float right = readBack.getSample(1, i);
        REQUIRE(left == right);
        REQUIRE(std::fabs(left - input.getSample(0, i)) < 1e-4f);
    }

    reader.reset();
    tmp.deleteFile();
}

TEST_CASE("WavWriter: file size scales with bit depth", "[pattern][wav]")
{
    const auto input = makeTestSine(440.0);
    const auto tmp8  = juce::File::createTempFile(".wav");
    const auto tmp16 = juce::File::createTempFile(".wav");
    const auto tmp24 = juce::File::createTempFile(".wav");

    REQUIRE(writeWav(input, kSampleRate, BitDepth::Eight,      ChannelMode::Mono, tmp8));
    REQUIRE(writeWav(input, kSampleRate, BitDepth::Sixteen,    ChannelMode::Mono, tmp16));
    REQUIRE(writeWav(input, kSampleRate, BitDepth::TwentyFour, ChannelMode::Mono, tmp24));

    REQUIRE(tmp8.getSize()  > 0);
    REQUIRE(tmp16.getSize() > tmp8.getSize());
    REQUIRE(tmp24.getSize() > tmp16.getSize());

    tmp8.deleteFile();
    tmp16.deleteFile();
    tmp24.deleteFile();
}

TEST_CASE("WavWriter: returns false when the destination cannot be opened",
          "[pattern][wav]")
{
    const auto input = makeTestSine(440.0);

    // A path inside a definitely-nonexistent directory cannot be opened.
    const juce::File bad { "/nonexistent_b33p_dir/should_not_open.wav" };
    REQUIRE_FALSE(writeWav(input, kSampleRate, BitDepth::Sixteen,
                           ChannelMode::Mono, bad));
}
