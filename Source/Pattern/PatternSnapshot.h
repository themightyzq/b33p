#pragma once

#include "Pattern.h"

#include <vector>

namespace B33p
{
    // One scheduled event in a snapshot — the underlying Event plus
    // the lane it belongs to, so the audio thread can route the
    // trigger to the correct per-lane Voice.
    struct ScheduledEvent
    {
        int   lane  { 0 };
        Event event;
    };

    // Read-only, audio-thread-safe view of a Pattern. Created on the
    // message thread (via makeSnapshot below) and handed to the audio
    // thread through a std::shared_ptr swap so concurrent UI edits to
    // the live Pattern can't corrupt mid-block reads.
    //
    // Events from all four lanes are flattened into a single vector
    // sorted by startSeconds, but each carries its lane index so
    // multi-voice playback can route each trigger to the right voice.
    // Events from muted lanes are filtered at snapshot time so the
    // audio thread never sees them; events whose startSeconds is
    // outside [0, lengthSeconds) are dropped.
    struct PatternSnapshot
    {
        double                      lengthSeconds { 0.0 };
        std::vector<ScheduledEvent> events;
    };

    PatternSnapshot makeSnapshot(const Pattern& pattern);
}
