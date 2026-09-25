#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "DSP/Voice.h"

#include <vector>

using B33p::Voice;
using Catch::Approx;

// Regression coverage for the Voice-level half of the "oversampler steps
// one sample at a time" fix: B33pProcessor now calls Voice::generateCore()
// per sample (for pattern-accurate trigger/noteOff timing) and then a
// single Voice::applyEffectsBlock() per chunk, instead of Voice::
// processSample() per sample. processSample() is specified to be exactly
// generateCore() + applyEffectsBlock() on a one-sample span, so these two
// call patterns must agree sample-for-sample.
namespace
{
    // Voice is non-copyable (owns a juce::dsp::IIR::Filter), so configure
    // it in place rather than returning by value.
    void configureVoice(Voice& v, double sampleRate)
    {
        v.prepare(sampleRate);
        v.setBasePitchHz(300.0f);
        v.setDistortionDrive(10.0f);
        v.setBitcrushBitDepth(4.0f);
        v.setBitcrushSampleRate(7000.0f);
    }
}

TEST_CASE("Voice: generateCore + batched applyEffectsBlock matches per-sample processSample()",
          "[dsp][voice][block]")
{
    constexpr double sampleRate = 48000.0;
    constexpr int    blockSize  = 512;
    constexpr int    numBlocks  = 6;
    constexpr int    numSamples = blockSize * numBlocks;

    Voice perSampleVoice;
    configureVoice(perSampleVoice, sampleRate);
    perSampleVoice.trigger(/*durationSeconds=*/1.0f, /*pitchOffsetSt=*/0.0f, /*velocity=*/0.8f);

    std::vector<float> perSampleOut(numSamples);
    for (int i = 0; i < numSamples; ++i)
        perSampleOut[static_cast<size_t>(i)] = perSampleVoice.processSample();

    Voice batchedVoice;
    configureVoice(batchedVoice, sampleRate);
    batchedVoice.trigger(1.0f, 0.0f, 0.8f);

    std::vector<float> batchedOut(numSamples);
    std::vector<float> scratch(blockSize);
    std::vector<float> velocity(blockSize);
    for (int b = 0; b < numBlocks; ++b)
    {
        const int base = b * blockSize;
        for (int i = 0; i < blockSize; ++i)
        {
            scratch[static_cast<size_t>(i)]  = batchedVoice.generateCore();
            velocity[static_cast<size_t>(i)] = batchedVoice.getTriggerVelocity();
        }
        batchedVoice.applyEffectsBlock(scratch.data(), velocity.data(), blockSize);
        for (int i = 0; i < blockSize; ++i)
            batchedOut[static_cast<size_t>(base + i)] = scratch[static_cast<size_t>(i)];
    }

    for (int i = 0; i < numSamples; ++i)
        REQUIRE(batchedOut[static_cast<size_t>(i)]
                == Approx(perSampleOut[static_cast<size_t>(i)]).margin(1e-6));
}

TEST_CASE("Voice: a mid-block retrigger's velocity change applies from the correct sample "
          "in the batched path",
          "[dsp][voice][block]")
{
    // Reproduces what B33pProcessor's per-sample pass 1 does when a pattern
    // event retriggers a lane partway through a chunk: trigger() (and its
    // new velocity) fires between two generateCore() calls, so the
    // velocity snapshot captured per sample must follow it exactly --
    // applying the OLD velocity to samples generated after the retrigger
    // (or the NEW velocity to samples generated before it) would be an
    // audible step at the wrong sample.
    constexpr double sampleRate  = 48000.0;
    constexpr int    blockSize   = 256;
    constexpr int    retriggerAt = 100;   // mid-block

    Voice perSampleVoice;
    configureVoice(perSampleVoice, sampleRate);
    perSampleVoice.trigger(1.0f, 0.0f, 0.3f);
    std::vector<float> perSampleOut(blockSize);
    for (int i = 0; i < blockSize; ++i)
    {
        if (i == retriggerAt)
            perSampleVoice.trigger(1.0f, 0.0f, 0.9f);
        perSampleOut[static_cast<size_t>(i)] = perSampleVoice.processSample();
    }

    Voice batchedVoice;
    configureVoice(batchedVoice, sampleRate);
    batchedVoice.trigger(1.0f, 0.0f, 0.3f);
    std::vector<float> scratch(blockSize);
    std::vector<float> velocity(blockSize);
    for (int i = 0; i < blockSize; ++i)
    {
        if (i == retriggerAt)
            batchedVoice.trigger(1.0f, 0.0f, 0.9f);
        scratch[static_cast<size_t>(i)]  = batchedVoice.generateCore();
        velocity[static_cast<size_t>(i)] = batchedVoice.getTriggerVelocity();
    }
    batchedVoice.applyEffectsBlock(scratch.data(), velocity.data(), blockSize);

    for (int i = 0; i < blockSize; ++i)
        REQUIRE(scratch[static_cast<size_t>(i)]
                == Approx(perSampleOut[static_cast<size_t>(i)]).margin(1e-6));
}
