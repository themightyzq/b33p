#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "Pattern/Pattern.h"
#include "Pattern/PatternSnapshot.h"

using B33p::Event;
using B33p::Pattern;
using B33p::PatternSnapshot;
using B33p::makeSnapshot;
using Catch::Approx;

TEST_CASE("PatternSnapshot: empty pattern produces an empty event list", "[pattern][snapshot]")
{
    Pattern p;
    const auto snap = makeSnapshot(p);
    REQUIRE(snap.lengthSeconds == Approx(p.getLengthSeconds()));
    REQUIRE(snap.events.empty());
}

TEST_CASE("PatternSnapshot: events from all lanes are flattened and sorted by startSeconds",
          "[pattern][snapshot]")
{
    Pattern p;
    p.addEvent(2, { 1.5, 0.1, 0.0f });
    p.addEvent(0, { 0.2, 0.1, 0.0f });
    p.addEvent(3, { 4.0, 0.1, 0.0f });
    p.addEvent(1, { 1.0, 0.1, 0.0f });

    const auto snap = makeSnapshot(p);

    REQUIRE(snap.events.size() == 4);
    REQUIRE(snap.events[0].event.startSeconds == Approx(0.2));
    REQUIRE(snap.events[1].event.startSeconds == Approx(1.0));
    REQUIRE(snap.events[2].event.startSeconds == Approx(1.5));
    REQUIRE(snap.events[3].event.startSeconds == Approx(4.0));
}

TEST_CASE("PatternSnapshot: events from a muted lane are dropped",
          "[pattern][snapshot]")
{
    Pattern p;
    p.addEvent(0, { 0.5, 0.1, 0.0f });
    p.addEvent(1, { 1.5, 0.1, 0.0f });
    p.addEvent(2, { 2.5, 0.1, 0.0f });
    p.setLaneMuted(1, true);

    const auto snap = makeSnapshot(p);
    REQUIRE(snap.events.size() == 2);
    // Only lanes 0 and 2 contribute — neither sits at 1.5s.
    for (const auto& e : snap.events)
        REQUIRE(e.event.startSeconds != Approx(1.5));
}

TEST_CASE("PatternSnapshot: solo overrides mute and silences non-soloed lanes",
          "[pattern][snapshot]")
{
    Pattern p;
    p.addEvent(0, { 0.5, 0.1, 0.0f });
    p.addEvent(1, { 1.0, 0.1, 0.0f });
    p.addEvent(2, { 1.5, 0.1, 0.0f });
    p.addEvent(3, { 2.0, 0.1, 0.0f });

    // Lane 1 muted but soloed: solo wins, lane 1 plays.
    // Lane 3 muted, not soloed: stays silent regardless.
    // Lanes 0 and 2: not soloed -> silent (because at least one
    // other lane IS soloed).
    p.setLaneMuted (1, true);
    p.setLaneSoloed(1, true);
    p.setLaneMuted (3, true);

    const auto snap = makeSnapshot(p);
    REQUIRE(snap.events.size() == 1);
    REQUIRE(snap.events[0].lane                 == 1);
    REQUIRE(snap.events[0].event.startSeconds   == Approx(1.0));
}

TEST_CASE("PatternSnapshot: events outside [0, length) are dropped", "[pattern][snapshot]")
{
    Pattern p;
    p.setLengthSeconds(2.0);

    p.addEvent(0, { -0.5, 0.1, 0.0f }); // negative start — drop
    p.addEvent(0, {  0.0, 0.1, 0.0f }); // edge inclusive
    p.addEvent(0, {  1.5, 0.1, 0.0f }); // inside — keep
    p.addEvent(0, {  2.0, 0.1, 0.0f }); // length boundary — drop
    p.addEvent(0, {  5.0, 0.1, 0.0f }); // way past — drop

    const auto snap = makeSnapshot(p);

    REQUIRE(snap.events.size() == 2);
    REQUIRE(snap.events[0].event.startSeconds == Approx(0.0));
    REQUIRE(snap.events[1].event.startSeconds == Approx(1.5));
}

TEST_CASE("PatternSnapshot: per-event metadata is preserved",
          "[pattern][snapshot]")
{
    Pattern p;
    p.addEvent(1, { 0.5, 0.25, 7.0f });

    const auto snap = makeSnapshot(p);

    REQUIRE(snap.events.size() == 1);
    REQUIRE(snap.events[0].event.startSeconds         == Approx(0.5));
    REQUIRE(snap.events[0].event.durationSeconds      == Approx(0.25));
    REQUIRE(snap.events[0].event.pitchOffsetSemitones == Approx(7.0f));
}

TEST_CASE("PatternSnapshot: probability = 0 always skips the event", "[pattern][snapshot]")
{
    B33p::Pattern pattern;
    B33p::Event ev;
    ev.startSeconds         = 0.5;
    ev.durationSeconds      = 0.2;
    ev.pitchOffsetSemitones = 0.0f;
    ev.velocity             = 1.0f;
    ev.probability          = 0.0f;
    pattern.addEvent(0, ev);

    juce::Random rng(42);
    for (int trial = 0; trial < 50; ++trial)
    {
        const auto snap = B33p::makeSnapshot(pattern, rng);
        REQUIRE(snap.events.empty());
    }
}

TEST_CASE("PatternSnapshot: probability = 1 always emits the event", "[pattern][snapshot]")
{
    B33p::Pattern pattern;
    B33p::Event ev;
    ev.startSeconds    = 0.5;
    ev.durationSeconds = 0.2;
    ev.probability     = 1.0f;
    pattern.addEvent(0, ev);

    juce::Random rng(42);
    for (int trial = 0; trial < 50; ++trial)
    {
        const auto snap = B33p::makeSnapshot(pattern, rng);
        REQUIRE(snap.events.size() == 1);
    }
}

TEST_CASE("PatternSnapshot: ratchets = N expands a parent into N evenly-spaced events",
          "[pattern][snapshot]")
{
    B33p::Pattern pattern;
    B33p::Event ev;
    ev.startSeconds    = 1.0;
    ev.durationSeconds = 1.0;
    ev.probability     = 1.0f;
    ev.ratchets        = 4;
    pattern.addEvent(0, ev);

    juce::Random rng(42);
    const auto snap = B33p::makeSnapshot(pattern, rng);

    REQUIRE(snap.events.size() == 4);
    for (size_t i = 0; i < snap.events.size(); ++i)
    {
        // Expected start time within ±5% of the perfect interval —
        // humanize=0 in this test so jitter is zero, but leave a
        // tiny margin for floating-point drift.
        const double expectedStart = 1.0 + 0.25 * static_cast<double>(i);
        REQUIRE(snap.events[i].event.startSeconds == Approx(expectedStart).margin(1e-6));
        REQUIRE(snap.events[i].event.durationSeconds == Approx(0.25).margin(1e-6));
        REQUIRE(snap.events[i].event.ratchets == 1);   // no recursive expansion
    }
}

TEST_CASE("PatternSnapshot: humanize jitter only softens velocity (never above original)",
          "[pattern][snapshot]")
{
    B33p::Pattern pattern;
    B33p::Event ev;
    ev.startSeconds    = 0.5;
    ev.durationSeconds = 0.1;
    ev.velocity        = 1.0f;
    ev.humanizeAmount  = 1.0f;   // maximum jitter
    pattern.addEvent(0, ev);

    juce::Random rng(7);
    bool sawDifference = false;
    for (int trial = 0; trial < 100; ++trial)
    {
        const auto snap = B33p::makeSnapshot(pattern, rng);
        REQUIRE(snap.events.size() == 1);
        const float v = snap.events.front().event.velocity;
        // Velocity must never exceed the original (humanize only
        // softens dynamics, it never boosts them) and must stay
        // non-negative.
        REQUIRE(v >= 0.0f);
        REQUIRE(v <= 1.0f);
        if (! juce::exactlyEqual(v, 1.0f))
            sawDifference = true;
    }
    // 100 trials with humanize=1 should produce many non-1 values.
    REQUIRE(sawDifference);
}
