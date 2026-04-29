#pragma once

#include "DSP/ModulationMatrix.h"
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

    // Global (non-lane-prefixed) parameter: a single 0..1 scale
    // applied to every random roll the ParameterRandomizer
    // produces. 1.0 = full range (current default); 0.1 = rolls
    // are constrained to ±10% of the parameter's current
    // normalised value, so the user can dial how "wild" the dice
    // get without locking parameters individually.
    inline juce::String randomizationScope() { return "randomization_scope"; }

    inline juce::String oscWaveform         (int lane) { return detail::prefixed(lane, "osc_waveform");           }
    inline juce::String basePitchHz         (int lane) { return detail::prefixed(lane, "base_pitch_hz");          }
    inline juce::String wavetableMorph      (int lane) { return detail::prefixed(lane, "wavetable_morph");        }
    inline juce::String fmRatio             (int lane) { return detail::prefixed(lane, "fm_ratio");               }
    inline juce::String fmDepth             (int lane) { return detail::prefixed(lane, "fm_depth");               }
    inline juce::String ringRatio           (int lane) { return detail::prefixed(lane, "ring_ratio");             }
    inline juce::String ringMix             (int lane) { return detail::prefixed(lane, "ring_mix");               }

    inline juce::String ampAttack           (int lane) { return detail::prefixed(lane, "amp_attack");             }
    inline juce::String ampDecay            (int lane) { return detail::prefixed(lane, "amp_decay");              }
    inline juce::String ampSustain          (int lane) { return detail::prefixed(lane, "amp_sustain");            }
    inline juce::String ampRelease          (int lane) { return detail::prefixed(lane, "amp_release");            }

    inline juce::String filterType          (int lane) { return detail::prefixed(lane, "filter_type");            }
    inline juce::String filterCutoffHz      (int lane) { return detail::prefixed(lane, "filter_cutoff_hz");       }
    inline juce::String filterResonanceQ    (int lane) { return detail::prefixed(lane, "filter_resonance_q");     }
    inline juce::String filterVowel         (int lane) { return detail::prefixed(lane, "filter_vowel");           }

    inline juce::String bitcrushBitDepth    (int lane) { return detail::prefixed(lane, "bitcrush_bit_depth");     }
    inline juce::String bitcrushSampleRateHz(int lane) { return detail::prefixed(lane, "bitcrush_sample_rate_hz"); }

    inline juce::String distortionDrive     (int lane) { return detail::prefixed(lane, "distortion_drive");       }

    inline juce::String modEffectType       (int lane) { return detail::prefixed(lane, "mod_effect_type");        }
    inline juce::String modEffectParam1     (int lane) { return detail::prefixed(lane, "mod_effect_p1");          }
    inline juce::String modEffectParam2     (int lane) { return detail::prefixed(lane, "mod_effect_p2");          }
    inline juce::String modEffectMix        (int lane) { return detail::prefixed(lane, "mod_effect_mix");         }

    inline juce::String voiceGain           (int lane) { return detail::prefixed(lane, "voice_gain");             }

    // LFO and modulation matrix per lane. lfo and slot indices are
    // 1-based in user-facing APVTS IDs to match the on-screen
    // labels ("LFO 1" / "Slot 1") — easier to grep saved files for.
    inline juce::String lfoShape            (int lane, int lfoIdx)
    {
        return detail::prefixed(lane, ("lfo" + juce::String(lfoIdx + 1) + "_shape").toRawUTF8());
    }
    inline juce::String lfoRateHz           (int lane, int lfoIdx)
    {
        return detail::prefixed(lane, ("lfo" + juce::String(lfoIdx + 1) + "_rate_hz").toRawUTF8());
    }
    inline juce::String modSlotSource       (int lane, int slotIdx)
    {
        return detail::prefixed(lane, ("mod" + juce::String(slotIdx + 1) + "_source").toRawUTF8());
    }
    inline juce::String modSlotDest         (int lane, int slotIdx)
    {
        return detail::prefixed(lane, ("mod" + juce::String(slotIdx + 1) + "_dest").toRawUTF8());
    }
    inline juce::String modSlotAmount       (int lane, int slotIdx)
    {
        return detail::prefixed(lane, ("mod" + juce::String(slotIdx + 1) + "_amount").toRawUTF8());
    }

    // Convenience iterator: every parameter ID for a single lane,
    // in the same order ParameterLayout registers them.
    inline std::vector<juce::String> allForLane(int lane)
    {
        std::vector<juce::String> ids = {
            oscWaveform(lane),         basePitchHz(lane),
            wavetableMorph(lane),
            fmRatio(lane),             fmDepth(lane),
            ringRatio(lane),           ringMix(lane),
            ampAttack(lane),           ampDecay(lane),
            ampSustain(lane),          ampRelease(lane),
            filterType(lane),
            filterCutoffHz(lane),      filterResonanceQ(lane),
            filterVowel(lane),
            bitcrushBitDepth(lane),    bitcrushSampleRateHz(lane),
            distortionDrive(lane),
            modEffectType(lane),       modEffectParam1(lane),
            modEffectParam2(lane),     modEffectMix(lane),
            voiceGain(lane)
        };
        for (int i = 0; i < kNumLfosPerLane; ++i)
        {
            ids.push_back(lfoShape (lane, i));
            ids.push_back(lfoRateHz(lane, i));
        }
        for (int i = 0; i < kNumModSlots; ++i)
        {
            ids.push_back(modSlotSource(lane, i));
            ids.push_back(modSlotDest  (lane, i));
            ids.push_back(modSlotAmount(lane, i));
        }
        return ids;
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
