#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace B33p
{
    // A single triggered beep inside a Pattern lane. startSeconds is
    // the absolute time from the start of the pattern (not relative to
    // anything else); durationSeconds is how long the note holds before
    // the voice's amp-envelope release kicks in; pitchOffsetSemitones
    // is applied on top of the voice's base pitch at trigger time.
    struct Event
    {
        double startSeconds         { 0.0 };
        double durationSeconds      { 0.1 };
        float  pitchOffsetSemitones { 0.0f };
    };

    // Plain-data pattern container. Four fixed lanes, each an ordered
    // vector of Events. No sorting, no overlap checks — the UI is free
    // to create events at any position, and the playback engine walks
    // the vector in whatever order it finds.
    //
    // Pattern length is user-adjustable within [kMin, kMax]; changing
    // length never touches existing events (per the scope call: don't
    // over-engineer). Events whose start is past the new length simply
    // do not play.
    //
    // Lane and event-index arguments are bounds-checked: out-of-range
    // calls are silent no-ops, and getEvents() with a bad lane returns
    // an empty reference. That keeps the UI layer terse without a
    // guard at every call site.
    class Pattern
    {
    public:
        static constexpr int    kNumLanes             = 4;
        static constexpr double kMinLengthSeconds     = 0.1;
        static constexpr double kMaxLengthSeconds     = 10.0;
        static constexpr double kDefaultLengthSeconds = 5.0;

        double getLengthSeconds() const noexcept;
        void   setLengthSeconds(double seconds);

        const std::vector<Event>& getEvents(int lane) const;

        void addEvent   (int lane, Event event);
        void removeEvent(int lane, std::size_t index);
        void updateEvent(int lane, std::size_t index, Event event);

        void clearLane(int lane);
        void clearAll();

    private:
        double                                      lengthSeconds { kDefaultLengthSeconds };
        std::array<std::vector<Event>, kNumLanes>   lanes;
    };
}
