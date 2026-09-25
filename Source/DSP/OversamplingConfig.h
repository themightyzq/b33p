#pragma once

namespace B33p
{
    // Single source of truth for the oversampling factor used by
    // OversampledDistortion and OversampledBitcrush to anti-alias the
    // Distortion waveshaper and the Bitcrush quantizer (both audibly
    // alias without it -- see TODO.md "Chores / tech debt").
    //
    // Must be a power of two: juce::dsp::Oversampling's constructor
    // takes a stage count N and gives 2^N times oversampling, so the
    // wrappers derive N from this value at construction.
    constexpr int kOversamplingFactor = 4;

    // Ceiling for one internal oversampler call in OversampledBitcrush /
    // OversampledDistortion's processBlock(). Chosen to match CLAUDE.md's
    // documented "Supported buffer sizes" contract (32-2048 samples), so
    // the overwhelmingly common case -- one host block -- is oversampled
    // in a single juce::dsp::Oversampling call pair instead of one pair
    // per single sample (the "oversampler steps one sample at a time"
    // finding; see CHANGELOG.md). Buffers sized to this are preallocated
    // in prepare(); processBlock() chunks internally at this size for
    // numSamples beyond it, so an out-of-contract call still processes
    // correctly with no additional allocation, just less amortization.
    constexpr int kMaxOversampledBlockSize = 2048;
}
