#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "DSP/OversampledBitcrush.h"
#include "DSP/OversampledDistortion.h"

#include <algorithm>
#include <cmath>
#include <vector>

using B33p::OversampledBitcrush;
using B33p::OversampledDistortion;
using Catch::Approx;

// Regression coverage for the "oversampler steps one sample at a time" fix
// (CHANGELOG.md): OversampledBitcrush / OversampledDistortion gained a
// processBlock() that batches juce::dsp::Oversampling's up/down filter pair
// across a whole chunk instead of calling it once per single sample. Both
// classes' internal state is a plain causal IIR recurrence, so batching
// must not change a single output value -- these tests assert the batched
// path reproduces the per-sample path exactly, at the block sizes and
// sample rates CLAUDE.md's audio contracts name (64/100/1024 samples,
// 44.1/48/96 kHz), plus a case that straddles a chunk boundary mid-note to
// prove chunking at kMaxOversampledBlockSize doesn't disturb state.
namespace
{
    std::vector<float> makeTestSignal(int numSamples, double sampleRate)
    {
        // A couple of summed sines plus a slow amplitude ramp -- enough
        // spectral and dynamic variety that a state-carrying bug (a reset
        // filter, a dropped sample, a misaligned oversampled tick) would
        // show up as a divergence somewhere in the run, not be masked by
        // a degenerate silent or DC input.
        std::vector<float> signal(static_cast<size_t>(numSamples));
        for (int i = 0; i < numSamples; ++i)
        {
            const double t = static_cast<double>(i) / sampleRate;
            const double envelope = 0.2 + 0.6 * (static_cast<double>(i) / numSamples);
            signal[static_cast<size_t>(i)] = static_cast<float>(
                envelope * (0.6 * std::sin(2.0 * juce::MathConstants<double>::pi * 440.0 * t)
                            + 0.3 * std::sin(2.0 * juce::MathConstants<double>::pi * 3000.0 * t)));
        }
        return signal;
    }

    std::vector<float> renderPerSample(OversampledDistortion& dist, const std::vector<float>& input)
    {
        std::vector<float> out(input.size());
        for (size_t i = 0; i < input.size(); ++i)
            out[i] = dist.processSample(input[i]);
        return out;
    }

    std::vector<float> renderPerSample(OversampledBitcrush& crush, const std::vector<float>& input)
    {
        std::vector<float> out(input.size());
        for (size_t i = 0; i < input.size(); ++i)
            out[i] = crush.processSample(input[i]);
        return out;
    }
}

TEST_CASE("OversampledDistortion: processBlock matches per-sample output at CLAUDE.md's tested "
          "block sizes and sample rates",
          "[dsp][distortion][oversampling][block]")
{
    const double sampleRates[] = { 44100.0, 48000.0, 96000.0 };
    const int    blockSizes[]  = { 64, 100, 1024 };   // 100 is the non-power-of-two

    for (double sampleRate : sampleRates)
    {
        for (int blockSize : blockSizes)
        {
            INFO("sampleRate=" << sampleRate << " blockSize=" << blockSize);

            const int numSamples = blockSize * 5;   // several chunks worth
            const auto input = makeTestSignal(numSamples, sampleRate);

            OversampledDistortion perSampleDist;
            perSampleDist.prepare(sampleRate);
            perSampleDist.setDrive(6.0f);
            const auto perSampleOut = renderPerSample(perSampleDist, input);

            OversampledDistortion batchedDist;
            batchedDist.prepare(sampleRate);
            batchedDist.setDrive(6.0f);
            std::vector<float> batchedOut = input;
            int offset = 0;
            while (offset < numSamples)
            {
                const int chunk = std::min(blockSize, numSamples - offset);
                batchedDist.processBlock(batchedOut.data() + offset, chunk);
                offset += chunk;
            }

            for (int i = 0; i < numSamples; ++i)
                REQUIRE(batchedOut[static_cast<size_t>(i)]
                        == Approx(perSampleOut[static_cast<size_t>(i)]).margin(1e-6));
        }
    }
}

TEST_CASE("OversampledBitcrush: processBlock matches per-sample output at CLAUDE.md's tested "
          "block sizes and sample rates",
          "[dsp][bitcrush][oversampling][block]")
{
    const double sampleRates[] = { 44100.0, 48000.0, 96000.0 };
    const int    blockSizes[]  = { 64, 100, 1024 };

    for (double sampleRate : sampleRates)
    {
        for (int blockSize : blockSizes)
        {
            INFO("sampleRate=" << sampleRate << " blockSize=" << blockSize);

            const int numSamples = blockSize * 5;
            const auto input = makeTestSignal(numSamples, sampleRate);

            OversampledBitcrush perSampleCrush;
            perSampleCrush.prepare(sampleRate);
            perSampleCrush.setBitDepth(5.0f);
            perSampleCrush.setTargetSampleRate(6000.0f);
            const auto perSampleOut = renderPerSample(perSampleCrush, input);

            OversampledBitcrush batchedCrush;
            batchedCrush.prepare(sampleRate);
            batchedCrush.setBitDepth(5.0f);
            batchedCrush.setTargetSampleRate(6000.0f);
            std::vector<float> batchedOut = input;
            int offset = 0;
            while (offset < numSamples)
            {
                const int chunk = std::min(blockSize, numSamples - offset);
                batchedCrush.processBlock(batchedOut.data() + offset, chunk);
                offset += chunk;
            }

            for (int i = 0; i < numSamples; ++i)
                REQUIRE(batchedOut[static_cast<size_t>(i)]
                        == Approx(perSampleOut[static_cast<size_t>(i)]).margin(1e-6));
        }
    }
}

TEST_CASE("OversampledDistortion: processBlock spanning more than kMaxOversampledBlockSize "
          "samples in one call chunks internally without disturbing state",
          "[dsp][distortion][oversampling][block]")
{
    // Exercises the internal chunking loop itself (a single processBlock()
    // call larger than the preallocated ceiling) rather than the caller
    // (B33pProcessor) chunking -- both paths must agree with per-sample.
    constexpr double sampleRate = 48000.0;
    const int numSamples = B33p::kMaxOversampledBlockSize + 500;
    const auto input = makeTestSignal(numSamples, sampleRate);

    OversampledDistortion perSampleDist;
    perSampleDist.prepare(sampleRate);
    perSampleDist.setDrive(3.0f);
    const auto perSampleOut = renderPerSample(perSampleDist, input);

    OversampledDistortion batchedDist;
    batchedDist.prepare(sampleRate);
    batchedDist.setDrive(3.0f);
    std::vector<float> batchedOut = input;
    batchedDist.processBlock(batchedOut.data(), numSamples);   // one oversized call

    for (int i = 0; i < numSamples; ++i)
        REQUIRE(batchedOut[static_cast<size_t>(i)]
                == Approx(perSampleOut[static_cast<size_t>(i)]).margin(1e-6));
}
