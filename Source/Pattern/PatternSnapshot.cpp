#include "PatternSnapshot.h"

#include <algorithm>

namespace B33p
{
    PatternSnapshot makeSnapshot(const Pattern& pattern)
    {
        PatternSnapshot snap;
        snap.lengthSeconds = pattern.getLengthSeconds();

        for (int lane = 0; lane < Pattern::kNumLanes; ++lane)
        {
            for (const auto& e : pattern.getEvents(lane))
            {
                if (e.startSeconds < 0.0 || e.startSeconds >= snap.lengthSeconds)
                    continue;
                snap.events.push_back(e);
            }
        }

        std::sort(snap.events.begin(), snap.events.end(),
            [](const Event& a, const Event& b)
            {
                return a.startSeconds < b.startSeconds;
            });

        return snap;
    }
}
