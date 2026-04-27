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
