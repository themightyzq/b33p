// Catch2 pipeline sanity checks. Once Phase 1 DSP tests land, this file can be
// replaced — for now it proves the test executable builds, runs, and reports
// failures back to ctest / CI.

#include <catch2/catch_test_macros.hpp>

TEST_CASE("sanity: arithmetic still works", "[sanity]")
{
    REQUIRE(1 + 1 == 2);
}

TEST_CASE("pipeline verification: intentional failure", "[sanity][pipeline-verification]")
{
    // This fail is deliberate — it proves CI actually executes tests rather
    // than silently skipping them. Flipped to a passing assertion in the
    // follow-up commit once the red CI run is observed.
    REQUIRE(1 == 2);
}
