#include "PatternSnapshot.h"

#include <algorithm>

namespace B33p
{
    PatternSnapshot makeSnapshot(const Pattern& pattern)
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
                snap.events.push_back({ lane, e });
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
