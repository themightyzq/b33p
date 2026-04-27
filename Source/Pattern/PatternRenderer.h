#pragma once

#include "DSP/Oscillator.h"
#include "DSP/PitchEnvelope.h"
#include "Pattern.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <vector>

namespace B33p
{
    // Offline-renders a Pattern + voice configuration to a mono
    // AudioBuffer<float>. No live audio device, no APVTS coupling —
    // takes a plain Config struct so the export dialog can populate
    // it from APVTS (or tests can populate it from literal values)
    // and the renderer stays self-contained.
    //
    // Algorithm matches B33pProcessor's live playback path:
    //   * walk the snapshot sample-by-sample
    //   * fire events whose startSeconds is at or before the current
    //     time, scheduling a noteOff after each event's duration
    //   * past the pattern length, ensure the voice receives noteOff
    //     and continue rendering until isActive() is false (release
    //     tail) or maxTailSeconds elapses, whichever comes first
    //   * trim the returned buffer to the actually-rendered length
    //
    // Determinism: same Pattern + same Config → same float samples,
    // EXCEPT for the noise oscillator whose RNG is seeded
    // non-deterministically (per the DSP convention; documented on
    // Oscillator).
    class PatternRenderer
    {
    public:
        // Per-lane voice configuration. Each lane in the rendered
        // pattern has its own voice with these settings; the four
        // voices are mixed sample-by-sample.
        struct LaneConfig
        {
            Oscillator::Waveform waveform { Oscillator::Waveform::Sine };
            float basePitchHz             { 440.0f };

            float ampAttack  { 0.005f };
            float ampDecay   { 0.05f  };
            float ampSustain { 1.0f   };
            float ampRelease { 0.1f   };

            float filterCutoffHz   { 20000.0f };
            float filterResonanceQ { 0.707f   };

            float bitcrushBitDepth     { 16.0f    };
            float bitcrushSampleRateHz { 48000.0f };

            float distortionDrive { 1.0f };
            float gain            { 1.0f };
        };

        struct Config
        {
            double sampleRate     { 48000.0 };
            double maxTailSeconds { 5.0 };

            // Pitch curve is shared across all four lanes for now —
            // matches the live processor. Per-lane curves are
            // post-MVP polish.
            std::vector<PitchEnvelopePoint> pitchCurve;

            std::array<LaneConfig, Pattern::kNumLanes> lanes;
        };

        // Renders the pattern to a single-channel float buffer.
        // The buffer length is exactly the rendered sample count
        // (pattern samples + however much tail the voices produce
        // before going inactive, capped at maxTailSeconds).
        static juce::AudioBuffer<float> render(const Pattern& pattern,
                                               const Config& config);
    };
}
