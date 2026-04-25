#pragma once

#include "Pattern.h"

#include <vector>

namespace B33p
{
    // Read-only, audio-thread-safe view of a Pattern. Created on the
    // message thread (via makeSnapshot below) and handed to the audio
    // thread through a std::shared_ptr swap so concurrent UI edits to
    // the live Pattern can't corrupt mid-block reads.
    //
    // Events from all four lanes are flattened into a single vector
    // sorted by startSeconds — the playback engine has one Voice and
    // does not care which lane an event came from. Events whose
    // startSeconds is outside [0, lengthSeconds) are dropped.
    struct PatternSnapshot
    {
        double             lengthSeconds { 0.0 };
        std::vector<Event> events;
    };

    PatternSnapshot makeSnapshot(const Pattern& pattern);
}
