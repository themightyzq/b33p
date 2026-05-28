#pragma once

#include "DSP/ModulationMatrix.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace B33p
{
    // UI-side helper for the modulation-glow halo on knobs (Phase B1 of
    // the "see what's modulating at a glance" work; see project memory
    // [[principle-nothing-hidden-see-at-a-glance]]). Given the current
    // LFO mirror values and the matrix configuration, computes a 0..1
    // intensity for any modulation destination on a lane — that
    // intensity drives `LabeledSlider::setModulationIntensity`, which
    // in turn drives the glow `B33pLookAndFeel::drawRotarySlider` paints
    // around the knob.
    //
    // Pure read-side: takes a non-const APVTS only because the JUCE
    // accessor isn't const-correct; never writes through it.
    namespace ModulationGlow
    {
        // Returns the matrix-driven modulation intensity 0..1 for
        // `destination` on `lane`, summing |lfo_value * slot_amount|
        // across every active slot that targets the destination. The
        // sum is clamped at 1 so stacked slots can't go super-bright.
        // Slots with source == None contribute nothing.
        float computeMatrixIntensity(juce::AudioProcessorValueTreeState& apvts,
                                     int lane,
                                     ModDestination destination,
                                     float lfo1Value,
                                     float lfo2Value);
    }
}
