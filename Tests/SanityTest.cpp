// Catch2 pipeline sanity checks. Replace with real DSP tests as Phase 1
// progresses — this file only exists to prove the test executable builds,
// runs, and reports failures back to ctest / CI.

#include <catch2/catch_test_macros.hpp>

TEST_CASE("sanity: arithmetic still works", "[sanity]")
{
    REQUIRE(1 + 1 == 2);
}

TEST_CASE("pipeline verification: failures surface in CI", "[sanity][pipeline-verification]")
{
    // The previous commit intentionally failed this assertion and the red
    // CI run was observed on all three OS runners, confirming that
    // catch_discover_tests wires up, ctest runs, and non-zero exit
    // propagates through GitHub Actions.
    REQUIRE(1 == 1);
}
