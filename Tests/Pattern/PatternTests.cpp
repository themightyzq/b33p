#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "Pattern/Pattern.h"

using B33p::Event;
using B33p::Pattern;
using Catch::Approx;

TEST_CASE("Pattern: default construction has sane state", "[pattern]")
{
    Pattern p;
    REQUIRE(p.getLengthSeconds() == Approx(Pattern::kDefaultLengthSeconds));
    for (int lane = 0; lane < Pattern::kNumLanes; ++lane)
        REQUIRE(p.getEvents(lane).empty());
}

TEST_CASE("Pattern: setLengthSeconds clamps into [kMin, kMax]", "[pattern]")
{
    Pattern p;

    p.setLengthSeconds(0.0);
    REQUIRE(p.getLengthSeconds() == Approx(Pattern::kMinLengthSeconds));

    p.setLengthSeconds(-1.5);
    REQUIRE(p.getLengthSeconds() == Approx(Pattern::kMinLengthSeconds));

    p.setLengthSeconds(100.0);
    REQUIRE(p.getLengthSeconds() == Approx(Pattern::kMaxLengthSeconds));

    p.setLengthSeconds(3.0);
    REQUIRE(p.getLengthSeconds() == Approx(3.0));
}

TEST_CASE("Pattern: addEvent appends to the requested lane only", "[pattern]")
{
    Pattern p;
    p.addEvent(0, { 1.0, 0.2, 0.0f  });
    p.addEvent(0, { 2.0, 0.3, 5.0f  });
    p.addEvent(2, { 0.5, 0.1, -7.0f });

    const auto& lane0 = p.getEvents(0);
    REQUIRE(lane0.size() == 2);
    REQUIRE(lane0[0].startSeconds          == Approx(1.0));
    REQUIRE(lane0[1].startSeconds          == Approx(2.0));
    REQUIRE(lane0[1].pitchOffsetSemitones  == Approx(5.0f));

    REQUIRE(p.getEvents(1).empty());
    REQUIRE(p.getEvents(2).size() == 1);
    REQUIRE(p.getEvents(2)[0].pitchOffsetSemitones == Approx(-7.0f));
    REQUIRE(p.getEvents(3).empty());
}

TEST_CASE("Pattern: removeEvent erases by index", "[pattern]")
{
    Pattern p;
    p.addEvent(0, { 1.0, 0.2, 0.0f });
    p.addEvent(0, { 2.0, 0.3, 0.0f });
    p.addEvent(0, { 3.0, 0.4, 0.0f });

    p.removeEvent(0, 1);

    const auto& lane0 = p.getEvents(0);
    REQUIRE(lane0.size() == 2);
    REQUIRE(lane0[0].startSeconds == Approx(1.0));
    REQUIRE(lane0[1].startSeconds == Approx(3.0));
}

TEST_CASE("Pattern: updateEvent modifies in place", "[pattern]")
{
    Pattern p;
    p.addEvent(0, { 1.0, 0.2, 0.0f });
    p.updateEvent(0, 0, { 1.5, 0.25, 3.0f });

    const auto& lane0 = p.getEvents(0);
    REQUIRE(lane0.size() == 1);
    REQUIRE(lane0[0].startSeconds          == Approx(1.5));
    REQUIRE(lane0[0].durationSeconds       == Approx(0.25));
    REQUIRE(lane0[0].pitchOffsetSemitones  == Approx(3.0f));
}

TEST_CASE("Pattern: clearLane empties one lane without touching others", "[pattern]")
{
    Pattern p;
    p.addEvent(0, { 1.0, 0.2, 0.0f });
    p.addEvent(1, { 2.0, 0.3, 0.0f });

    p.clearLane(0);
    REQUIRE(p.getEvents(0).empty());
    REQUIRE(p.getEvents(1).size() == 1);
}

TEST_CASE("Pattern: clearAll empties every lane", "[pattern]")
{
    Pattern p;
    for (int lane = 0; lane < Pattern::kNumLanes; ++lane)
        p.addEvent(lane, { 1.0, 0.2, 0.0f });

    p.clearAll();
    for (int lane = 0; lane < Pattern::kNumLanes; ++lane)
        REQUIRE(p.getEvents(lane).empty());
}

TEST_CASE("Pattern: out-of-range lanes and indices are silent no-ops", "[pattern]")
{
    Pattern p;

    // Reads
    REQUIRE(p.getEvents(-1).empty());
    REQUIRE(p.getEvents(999).empty());

    // Writes — should neither throw nor corrupt state.
    p.addEvent(-1, { 1.0, 0.2, 0.0f });
    p.addEvent(999, { 1.0, 0.2, 0.0f });
    p.removeEvent(-1, 0);
    p.removeEvent(999, 0);
    p.removeEvent(0, 999);      // valid lane, bad index
    p.updateEvent(-1, 0, {});
    p.updateEvent(999, 0, {});
    p.updateEvent(0, 999, {});  // valid lane, bad index
    p.clearLane(-1);
    p.clearLane(999);

    for (int lane = 0; lane < Pattern::kNumLanes; ++lane)
        REQUIRE(p.getEvents(lane).empty());
}
