#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "Pattern/SnapMath.h"

using B33p::snapToGrid;
using Catch::Approx;

TEST_CASE("snapToGrid: snaps to nearest multiple of grid", "[pattern][snap]")
{
    REQUIRE(snapToGrid(0.13, 0.1)  == Approx(0.1));
    REQUIRE(snapToGrid(0.16, 0.1)  == Approx(0.2));
    REQUIRE(snapToGrid(0.50, 0.1)  == Approx(0.5));
    REQUIRE(snapToGrid(1.00, 0.25) == Approx(1.0));
    REQUIRE(snapToGrid(1.30, 0.25) == Approx(1.25));
    REQUIRE(snapToGrid(1.40, 0.25) == Approx(1.5));
}

TEST_CASE("snapToGrid: zero stays at zero", "[pattern][snap]")
{
    REQUIRE(snapToGrid(0.0, 0.1)   == Approx(0.0));
    REQUIRE(snapToGrid(0.0, 1.0)   == Approx(0.0));
}

TEST_CASE("snapToGrid: gridSeconds <= 0 disables snap", "[pattern][snap]")
{
    REQUIRE(snapToGrid(0.137, 0.0)  == Approx(0.137));
    REQUIRE(snapToGrid(0.137, -1.0) == Approx(0.137));
    REQUIRE(snapToGrid(9.876, 0.0)  == Approx(9.876));
}

TEST_CASE("snapToGrid: negative inputs snap symmetrically", "[pattern][snap]")
{
    REQUIRE(snapToGrid(-0.13, 0.1) == Approx(-0.1));
    REQUIRE(snapToGrid(-0.16, 0.1) == Approx(-0.2));
    REQUIRE(snapToGrid(-0.50, 0.1) == Approx(-0.5));
}

TEST_CASE("snapToGrid: large grids handle long times", "[pattern][snap]")
{
    REQUIRE(snapToGrid(7.7, 1.0) == Approx(8.0));
    REQUIRE(snapToGrid(2.4, 1.0) == Approx(2.0));
    REQUIRE(snapToGrid(2.5, 1.0) == Approx(3.0));   // round-half-away-from-zero
}

TEST_CASE("snapToGrid: exact midpoints round half away from zero", "[pattern][snap]")
{
    // Inputs and grid sizes are negative powers of two so the
    // arithmetic is exact in IEEE-754 — using e.g. 0.15 / 0.1 here
    // would actually snap down because 0.15 in double is
    // 0.14999..., not the textbook midpoint we'd want to test.
    REQUIRE(snapToGrid( 0.25, 0.5) == Approx( 0.5));
    REQUIRE(snapToGrid( 0.75, 0.5) == Approx( 1.0));
    REQUIRE(snapToGrid( 1.25, 0.5) == Approx( 1.5));
    REQUIRE(snapToGrid(-0.25, 0.5) == Approx(-0.5));
    REQUIRE(snapToGrid(-0.75, 0.5) == Approx(-1.0));
}
