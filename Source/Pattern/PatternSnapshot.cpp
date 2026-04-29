#include "PatternSnapshot.h"

#include <algorithm>

namespace B33p
{
    namespace
    {
        // Returns a uniform float in [0, 1).
        inline float uniform01(juce::Random& rng)
        {
            return rng.nextFloat();
        }

        // Returns a symmetric jitter in [-1, +1).
        inline float jitterPM1(juce::Random& rng)
        {
            return rng.nextFloat() * 2.0f - 1.0f;
        }

        // Apply humanize-style jitter to start time + velocity. The
        // start-time jitter is bounded by a small fraction of the
        // event's duration so a humanized event never crosses into
        // the next slot, even at humanize=1. Velocity jitter scales
        // the existing velocity by a one-sided random multiplier so
        // humanize never *increases* loudness — it can only soften.
        void applyHumanize(Event& e, float humanizeAmount, juce::Random& rng)
        {
            const float clamped = juce::jlimit(0.0f, 1.0f, humanizeAmount);
            if (clamped <= 0.0f)
                return;

            // Timing: ±25% of duration at full humanize.
            const double timeJitter = static_cast<double>(jitterPM1(rng))
                                       * static_cast<double>(clamped) * 0.25
                                       * e.durationSeconds;
            e.startSeconds = std::max(0.0, e.startSeconds + timeJitter);

            // Velocity: scale by [1 - clamped, 1] uniformly.
            const float velScale = 1.0f - clamped * uniform01(rng);
            e.velocity = juce::jlimit(0.0f, 1.0f, e.velocity * velScale);
        }

        // Emits the event (or N ratchets) into the snapshot if its
        // probability roll succeeds. ratchets > 1 splits the event
        // into N back-to-back shorter hits sharing the parent's
        // overrides / pitch / velocity, then humanize is applied
        // independently per ratchet.
        void emitEventInto(PatternSnapshot& snap,
                            int lane,
                            const Event& parent,
                            juce::Random& rng)
        {
            const float prob = juce::jlimit(0.0f, 1.0f, parent.probability);
            if (prob < 1.0f && uniform01(rng) >= prob)
                return;

            const int ratchets = juce::jlimit(1, kMaxRatchets, parent.ratchets);
            const double ratchetDuration = parent.durationSeconds
                                              / static_cast<double>(ratchets);

            for (int r = 0; r < ratchets; ++r)
            {
                Event e = parent;
                e.startSeconds    = parent.startSeconds
                                       + ratchetDuration * static_cast<double>(r);
                e.durationSeconds = ratchetDuration;
                // Avoid recursive expansion when this snapshot is
                // ever re-fed through makeSnapshot.
                e.ratchets        = 1;
                applyHumanize(e, parent.humanizeAmount, rng);

                if (e.startSeconds < 0.0 || e.startSeconds >= snap.lengthSeconds)
                    continue;
                snap.events.push_back({ lane, e });
            }
        }
    }

    PatternSnapshot makeSnapshot(const Pattern& pattern, juce::Random& rng)
    {
        PatternSnapshot snap;
        snap.lengthSeconds = pattern.getLengthSeconds();

        // While any lane is soloed, only soloed lanes contribute —
        // mute is ignored on soloed lanes (solo wins). Otherwise
        // mute alone gates output. Filtering happens at snapshot
        // time so the audio thread doesn't need lane bookkeeping.
        const bool anySoloed = pattern.anyLaneSoloed();

        for (int lane = 0; lane < Pattern::kNumLanes; ++lane)
        {
            const bool include = anySoloed
                                     ? pattern.isLaneSoloed(lane)
                                     : ! pattern.isLaneMuted(lane);
            if (! include)
                continue;

            for (const auto& e : pattern.getEvents(lane))
            {
                if (e.startSeconds < 0.0 || e.startSeconds >= snap.lengthSeconds)
                    continue;
                emitEventInto(snap, lane, e, rng);
            }
        }

        std::sort(snap.events.begin(), snap.events.end(),
            [](const ScheduledEvent& a, const ScheduledEvent& b)
            {
                return a.event.startSeconds < b.event.startSeconds;
            });

        return snap;
    }
}
