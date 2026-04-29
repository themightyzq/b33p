#pragma once

namespace B33p
{
    // Modulation source: which LFO (or none) drives a matrix slot.
    // Values match the APVTS Choice index for mod_slotN_source.
    enum class ModSource : int
    {
        None = 0,
        LFO1,
        LFO2,
    };
    constexpr int kNumModSources = 3;

    // Modulation destination: which voice parameter a matrix slot
    // shifts. Values match the APVTS Choice index for
    // mod_slotN_dest. Each destination's "natural range" is the
    // APVTS parameter's full range, and matrix amount [-1, 1] is
    // applied as a normalised offset (clamped) — see the
    // applyModulationToParameter helper in B33pProcessor.cpp.
    enum class ModDestination : int
    {
        None = 0,
        OscBasePitch,
        WavetableMorph,
        FmDepth,
        RingMix,
        FilterCutoff,
        FilterResonance,
        DistortionDrive,
        ModEffectParam1,
        ModEffectParam2,
        ModEffectMix,
        VoiceGain,
    };
    constexpr int kNumModDestinations = 12;

    // Number of modulation matrix slots per lane. A user with two
    // LFOs and four slots can route both LFOs at once and still
    // have headroom for a couple of static offsets — enough variety
    // for most patches without overflowing the UI.
    constexpr int kNumModSlots = 4;

    // Number of free-running LFOs per lane.
    constexpr int kNumLfosPerLane = 2;
}
