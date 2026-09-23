#include <catch2/catch_test_macros.hpp>

#include "DSP/OversampledBitcrush.h"
#include "DSP/OversampledDistortion.h"
#include "DSP/Voice.h"
#include "State/B33pProcessor.h"

using B33p::B33pProcessor;
using B33p::OversampledBitcrush;
using B33p::OversampledDistortion;
using B33p::Voice;

namespace
{
    int latencyAt(double sampleRate, int blockSize)
    {
        B33pProcessor processor;
        processor.prepareToPlay(sampleRate, blockSize);
        return processor.getLatencySamples();
    }
}

TEST_CASE("Voice: reported latency is the sum of the Distortion + Bitcrush oversampling stages",
          "[dsp][voice][latency]")
{
    constexpr double sampleRate = 48000.0;

    Voice voice;
    voice.prepare(sampleRate);

    OversampledDistortion dist;
    dist.prepare(sampleRate);
    OversampledBitcrush crush;
    crush.prepare(sampleRate);

    REQUIRE(voice.getLatencySamples() == dist.getLatencySamples() + crush.getLatencySamples());
}

TEST_CASE("B33pProcessor: reported latency is nonzero now the nonlinear stages are oversampled",
          "[state][latency]")
{
    // Catches an accidental revert to the old hardcoded
    // setLatencySamples(0) from before oversampling existed.
    B33pProcessor processor;
    processor.prepareToPlay(48000.0, 256);
    REQUIRE(processor.getLatencySamples() > 0);
}

TEST_CASE("B33pProcessor: reported latency equals a lane voice's own reported latency",
          "[state][latency]")
{
    constexpr double sampleRate = 48000.0;

    B33pProcessor processor;
    processor.prepareToPlay(sampleRate, 256);

    Voice voice;
    voice.prepare(sampleRate);

    REQUIRE(processor.getLatencySamples() == voice.getLatencySamples());
}

TEST_CASE("B33pProcessor: reported latency is identical across buffer sizes 64/100/512/1024 "
          "and sample rates 44.1/48/96 kHz",
          "[state][latency]")
{
    // juce::dsp::Oversampling's filter design takes no sample-rate or
    // block-size parameter, so the latency it reports in samples should
    // be the same constant everywhere -- CLAUDE.md's "keep it block-size
    // invariant" contract, extended here to also cover sample rate
    // since nothing in the design should make it vary.
    const int reference = latencyAt(48000.0, 256);
    REQUIRE(reference > 0);

    const double sampleRates[] = { 44100.0, 48000.0, 96000.0 };
    const int    blockSizes[]  = { 64, 100, 512, 1024 };   // 100 is the non-power-of-two

    for (double sr : sampleRates)
        for (int blockSize : blockSizes)
            REQUIRE(latencyAt(sr, blockSize) == reference);
}
