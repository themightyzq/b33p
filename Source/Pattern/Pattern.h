#pragma once

#include "DSP/ModulationMatrix.h"

#include <juce_core/juce_core.h>

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace B33p
{
    // Number of per-event override slots. Four matches the
    // modulation matrix slot count — a single event can pin any
    // four voice parameters to specific values for that hit only,
    // overriding both the lane's APVTS base values and the LFO
    // matrix contribution.
    constexpr int kNumEventOverrides = 4;

    // A single per-event parameter override. value is in normalised
    // [0, 1] (the same convention the modulation matrix uses for
    // contributions); B33pProcessor converts it to the destination
    // parameter's natural range via convertFrom0to1 before pushing
    // to the Voice. destination = None disables the slot.
    struct EventOverride
    {
        ModDestination destination { ModDestination::None };
        float          value       { 0.0f };
    };

    inline bool operator==(const EventOverride& a, const EventOverride& b)
    {
        return a.destination == b.destination
            && juce::exactlyEqual(a.value, b.value);
    }
    inline bool operator!=(const EventOverride& a, const EventOverride& b) { return ! (a == b); }

    // Maximum ratchet count per event. 8 is plenty for the
    // "buzzing snare" and "16th-note flam" cases users typically
    // want; higher counts begin to crash into the amp envelope's
    // attack/release tails and stop sounding like rhythmic ratchet.
    constexpr int kMaxRatchets = 8;

    // A single triggered beep inside a Pattern lane. startSeconds is
    // the absolute time from the start of the pattern (not relative to
    // anything else); durationSeconds is how long the note holds before
    // the voice's amp-envelope release kicks in; pitchOffsetSemitones
    // is applied on top of the voice's base pitch at trigger time;
    // velocity is a 0..1 scalar applied on top of the voice's gain
    // (1.0 = unchanged, 0.5 = -6 dB, 0.0 = silent). overrides is a
    // small fixed-size set of per-event parameter pins — see
    // EventOverride for semantics.
    //
    // probability: 0..1 chance the event fires when its loop slot
    // arrives. Rolled at snapshot time, so a roll-out skips the
    // entire event (and any ratchets) until the next snapshot.
    // ratchets:    1..kMaxRatchets retriggers within the event's
    // duration. ratchets = 1 disables. Higher counts split the
    // event into N evenly-spaced shorter hits at snapshot time.
    // humanizeAmount: 0..1 random jitter applied to start time
    // and velocity at snapshot time. Re-randomises each time the
    // snapshot is rebuilt.
    struct Event
    {
        double startSeconds         { 0.0 };
        double durationSeconds      { 0.1 };
        float  pitchOffsetSemitones { 0.0f };
        float  velocity             { 1.0f };
        float  probability          { 1.0f };
        int    ratchets             { 1   };
        float  humanizeAmount       { 0.0f };
        std::array<EventOverride, kNumEventOverrides> overrides {};
    };

    // Bit-exact comparison — used for snapshot equality in undo,
    // not numeric tolerance. juce::exactlyEqual silences the
    // -Wfloat-equal warning that JUCE's recommended flags enable.
    inline bool operator==(const Event& a, const Event& b)
    {
        return juce::exactlyEqual(a.startSeconds,         b.startSeconds)
            && juce::exactlyEqual(a.durationSeconds,      b.durationSeconds)
            && juce::exactlyEqual(a.pitchOffsetSemitones, b.pitchOffsetSemitones)
            && juce::exactlyEqual(a.velocity,             b.velocity)
            && juce::exactlyEqual(a.probability,          b.probability)
            && a.ratchets == b.ratchets
            && juce::exactlyEqual(a.humanizeAmount,       b.humanizeAmount)
            && a.overrides == b.overrides;
    }
    inline bool operator!=(const Event& a, const Event& b) { return ! (a == b); }

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

        // Tempo + time signature defaults. The pattern still measures
        // event timing in seconds (the audio engine is sample-rate-
        // based, not tempo-based), but BPM and time signature are
        // first-class fields so the UI can offer musical snap values
        // and bars/beats display.
        static constexpr double kDefaultBpm            = 120.0;
        static constexpr double kMinBpm                = 20.0;
        static constexpr double kMaxBpm                = 999.0;
        static constexpr int    kDefaultTimeSigNum     = 4;
        static constexpr int    kDefaultTimeSigDen     = 4;

        double getLengthSeconds() const noexcept;
        void   setLengthSeconds(double seconds);

        double getBpm() const noexcept;
        void   setBpm(double bpm);

        int    getTimeSigNumerator()   const noexcept;
        int    getTimeSigDenominator() const noexcept;
        void   setTimeSignature(int numerator, int denominator);

        const std::vector<Event>& getEvents(int lane) const;

        void addEvent   (int lane, Event event);
        void removeEvent(int lane, std::size_t index);
        void updateEvent(int lane, std::size_t index, Event event);

        void clearLane(int lane);
        void clearAll();

        // Resets per-lane metadata (names, mute) to defaults.
        // clearAll only touches events; load / new-project flows
        // need this to wipe leftover names + mute from the
        // previous project's pattern.
        void resetAllLaneMeta();

        // Per-lane name. Empty string is the "use default label"
        // sentinel; the UI renders "1" / "2" / ... in that case.
        const juce::String& getLaneName(int lane) const;
        void                setLaneName(int lane, const juce::String& name);

        // Per-lane mute. Muted lanes are filtered out at snapshot
        // time so the audio thread never sees their events.
        bool isLaneMuted(int lane) const;
        void setLaneMuted(int lane, bool muted);

        // Per-lane solo. While ANY lane is soloed, the snapshot only
        // includes events from soloed lanes — non-soloed lanes are
        // silent regardless of their own mute state. With no lanes
        // soloed, mute alone gates lane output.
        bool isLaneSoloed(int lane) const;
        void setLaneSoloed(int lane, bool soloed);
        bool anyLaneSoloed() const;

        // Snapshot equality — used by the undo system to skip
        // pushing no-op gestures (e.g. mouseDown + mouseUp without
        // any actual change to the pattern data).
        friend bool operator==(const Pattern& a, const Pattern& b)
        {
            return juce::exactlyEqual(a.lengthSeconds, b.lengthSeconds)
                && juce::exactlyEqual(a.bpm,           b.bpm)
                && a.timeSigNumerator   == b.timeSigNumerator
                && a.timeSigDenominator == b.timeSigDenominator
                && a.lanes      == b.lanes
                && a.laneNames  == b.laneNames
                && a.laneMuted  == b.laneMuted
                && a.laneSoloed == b.laneSoloed;
        }
        friend bool operator!=(const Pattern& a, const Pattern& b) { return ! (a == b); }

    private:
        double                                      lengthSeconds { kDefaultLengthSeconds };
        double                                      bpm           { kDefaultBpm };
        int                                         timeSigNumerator   { kDefaultTimeSigNum };
        int                                         timeSigDenominator { kDefaultTimeSigDen };
        std::array<std::vector<Event>, kNumLanes>   lanes;
        std::array<juce::String, kNumLanes>         laneNames;
        std::array<bool, kNumLanes>                 laneMuted  { false, false, false, false };
        std::array<bool, kNumLanes>                 laneSoloed { false, false, false, false };
    };
}
