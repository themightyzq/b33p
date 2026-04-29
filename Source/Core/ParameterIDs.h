#pragma once

#include "Pattern/Pattern.h"

#include <juce_core/juce_core.h>

#include <vector>

namespace B33p::ParameterIDs
{
    // Lane-aware APVTS parameter identifiers. Each lane (0..kNumLanes-1)
    // owns its own complete parameter set so the four voices in a
    // multi-voice pattern can each carry independent timbres.
    //
    // The ID format is "lane{N}_{base}" — readable in the .beep XML
    // and picked up by ProjectState's v1 → v2 migration which fans
    // the single v1 voice's parameters out to every lane's set.
    //
    // Returning juce::String by value rather than constexpr const char*
    // because each call computes a per-lane string. juce::String's
    // small-string optimisation keeps construction cheap; the IDs are
    // captured at attachment / listener time, not on the audio thread.

    namespace detail
    {
        inline juce::String prefixed(int lane, const char* base)
        {
            return juce::String("lane") + juce::String(lane)
                 + juce::String("_")    + juce::String(base);
        }
    }

    inline juce::String oscWaveform         (int lane) { return detail::prefixed(lane, "osc_waveform");           }
    inline juce::String basePitchHz         (int lane) { return detail::prefixed(lane, "base_pitch_hz");          }
    inline juce::String wavetableMorph      (int lane) { return detail::prefixed(lane, "wavetable_morph");        }
    inline juce::String fmRatio             (int lane) { return detail::prefixed(lane, "fm_ratio");               }
    inline juce::String fmDepth             (int lane) { return detail::prefixed(lane, "fm_depth");               }

    inline juce::String ampAttack           (int lane) { return detail::prefixed(lane, "amp_attack");             }
    inline juce::String ampDecay            (int lane) { return detail::prefixed(lane, "amp_decay");              }
    inline juce::String ampSustain          (int lane) { return detail::prefixed(lane, "amp_sustain");            }
    inline juce::String ampRelease          (int lane) { return detail::prefixed(lane, "amp_release");            }

    inline juce::String filterCutoffHz      (int lane) { return detail::prefixed(lane, "filter_cutoff_hz");       }
    inline juce::String filterResonanceQ    (int lane) { return detail::prefixed(lane, "filter_resonance_q");     }

    inline juce::String bitcrushBitDepth    (int lane) { return detail::prefixed(lane, "bitcrush_bit_depth");     }
    inline juce::String bitcrushSampleRateHz(int lane) { return detail::prefixed(lane, "bitcrush_sample_rate_hz"); }

    inline juce::String distortionDrive     (int lane) { return detail::prefixed(lane, "distortion_drive");       }

    inline juce::String voiceGain           (int lane) { return detail::prefixed(lane, "voice_gain");             }

    // Convenience iterator: every parameter ID for a single lane,
    // in the same order ParameterLayout registers them.
    inline std::vector<juce::String> allForLane(int lane)
    {
        return {
            oscWaveform(lane),         basePitchHz(lane),
            wavetableMorph(lane),
            fmRatio(lane),             fmDepth(lane),
            ampAttack(lane),           ampDecay(lane),
            ampSustain(lane),          ampRelease(lane),
            filterCutoffHz(lane),      filterResonanceQ(lane),
            bitcrushBitDepth(lane),    bitcrushSampleRateHz(lane),
            distortionDrive(lane),     voiceGain(lane)
        };
    }

    // Per-parameter randomization gate. voiceGain is excluded
    // because a sudden +20 dB random jump is an excellent way to
    // blow out the user's ears or speakers. Randomize-style
    // entry points should consult this when picking targets.
    inline bool isRandomizable(const juce::String& id)
    {
        return ! id.endsWith("_voice_gain");
    }

    // Subset of allForLane filtered by isRandomizable — convenient
    // for the per-lane "Randomize Lane" button.
    inline std::vector<juce::String> randomizableForLane(int lane)
    {
        std::vector<juce::String> out;
        for (auto& id : allForLane(lane))
            if (isRandomizable(id))
                out.push_back(id);
        return out;
    }

    // The flat (lane-less) IDs that v0.1.0 .beep files used. ProjectState's
    // v1 → v2 migration looks for these and rewrites them into lane-prefixed
    // IDs across all four lanes.
    namespace v1
    {
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
}
