#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <vector>

namespace B33p
{
    // Helpers backing the project rule: every randomize button must yield
    // an audible (not silent or near-silent) patch. See the project
    // memories `rule-randomize-must-be-audible` and
    // `principle-nothing-hidden-see-at-a-glance` for the rule's intent.
    //
    // `ParameterRandomizer` rolls each parameter independently with no
    // whole-patch check, so silence emerges from combinations (filter
    // closed, amp sustain ~0, Mod FX wet kills the dry, ...). This
    // module supplies the three primitives the processor's audibility
    // guard composes: an offline single-lane peak render, the list of
    // level-critical parameters to re-roll when the patch is silent,
    // and a guaranteed-audible fallback for when re-rolling fails.
    namespace AudibilityCheck
    {
        // Linear peak below which we treat the patch as "near silent."
        // ~ -40 dBFS — a beep peaking quieter than this against typical
        // monitoring is effectively unheard.
        inline constexpr float kAudibilityFloorLinear = 0.01f;

        // Renders a `durationSeconds`-long single-note audition for
        // `lane` offline using a temporary `Voice` configured from
        // the current APVTS lane parameters, and returns the linear
        // peak. Pure offline; safe to call on the message thread.
        // Returns 0 for an out-of-range lane.
        //
        // Mirrors `B33pProcessor::pushParametersToLane`'s voice setter
        // sequence using the raw (unmodulated) APVTS values — LFO
        // matrix mod is omitted because over the audition duration it
        // oscillates around the base and rarely moves average peak.
        float measureLanePeakOffline(juce::AudioProcessorValueTreeState& apvts,
                                     int lane,
                                     double sampleRate,
                                     double durationSeconds);

        // The level-critical parameters on `lane` that the audibility
        // guard re-rolls when the offline render comes back too quiet.
        // (amp env shape, filter cutoff/resonance, Mod FX mix, and the
        // osc-mix params that can mute the carrier.)
        std::vector<juce::String> levelCriticalParamsForLane(int lane);

        // Final fallback when re-rolling fails to recover audibility:
        // forces `lane`'s level-critical parameters to known-audible
        // safe values (filter open at ~ 4 kHz, amp env with full
        // sustain and a short clean attack, Mod FX fully dry).
        // Writes through APVTS so the UI + undo system see the change.
        void forceLaneToAudibleFallback(juce::AudioProcessorValueTreeState& apvts,
                                        int lane);

        // True if the patch's measured peak is below the audibility
        // floor — convenience predicate so the policy code stays
        // readable.
        inline bool isNearSilent(float linearPeak)
        {
            return linearPeak < kAudibilityFloorLinear;
        }
    }
}
