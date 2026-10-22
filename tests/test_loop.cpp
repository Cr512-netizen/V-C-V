#include <catch2/catch_test_macros.hpp>
#include "app/loop.h"
TEST_CASE("fixed tick accumulates correctly at 60 Hz") {
 double a = 0;
 REQUIRE(nuc::Loop::consume_ticks(1.0/30.0, a, 1.0/60.0) == 2);
 REQUIRE(a == 0.0);
}
TEST_CASE("runtime loop supports the 15 Hz default step") {
 double a = 0;
 REQUIRE(nuc::Loop::consume_ticks(1.0/15.0, a, 1.0/15.0) == 1);
 REQUIRE(a == 0.0);
}
