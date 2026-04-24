#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace B33p
{
    // Builds the full APVTS parameter layout for a B33p voice: one
    // choice parameter (oscillator waveform) plus eleven continuous
    // parameters covering pitch, amp envelope, filter, bitcrush,
    // distortion, and master gain.
    //
    // Parameter IDs come from B33p::ParameterIDs and are versioned
    // with juce::ParameterID version hint 1. Ranges and defaults
    // mirror the clamps in the underlying DSP primitives.
    //
    // Skew centres are chosen so sliders feel musical: Hz ranges
    // concentrate around typical musical values, short-time envelope
    // parameters concentrate around 0.1 s, others stay linear.
    //
    // The drawable pitch envelope is intentionally NOT in the APVTS
    // layout — it is project-tree data (Phase 6), not a
    // host-automatable parameter.
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
}
