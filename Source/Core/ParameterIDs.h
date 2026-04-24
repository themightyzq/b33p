#pragma once

namespace B33p::ParameterIDs
{
    // Stable string identifiers for every APVTS parameter in the
    // project. Used by the UI, randomization system, and .beep project
    // serialization. Changing an ID breaks older saved projects —
    // always add a migration step if renaming is unavoidable.
    inline constexpr const char* oscWaveform             = "osc_waveform";
    inline constexpr const char* basePitchHz             = "base_pitch_hz";

    inline constexpr const char* ampAttack               = "amp_attack";
    inline constexpr const char* ampDecay                = "amp_decay";
    inline constexpr const char* ampSustain              = "amp_sustain";
    inline constexpr const char* ampRelease              = "amp_release";

    inline constexpr const char* filterCutoffHz          = "filter_cutoff_hz";
    inline constexpr const char* filterResonanceQ        = "filter_resonance_q";

    inline constexpr const char* bitcrushBitDepth        = "bitcrush_bit_depth";
    inline constexpr const char* bitcrushSampleRateHz    = "bitcrush_sample_rate_hz";

    inline constexpr const char* distortionDrive         = "distortion_drive";

    inline constexpr const char* voiceGain               = "voice_gain";
}
