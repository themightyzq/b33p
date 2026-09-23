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
}
