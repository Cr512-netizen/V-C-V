#include <catch2/catch_test_macros.hpp>
#include "core/sim/fixed_tick.h"
#include "core/sim/rng.h"
#include "core/sim/state_hash.h"
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

TEST_CASE("fixed tick defaults to 15 Hz and clamps catch-up") {
 nuc::sim::FixedTick clock;
 REQUIRE(std::abs(clock.step() - (1.0 / 15.0)) < 1e-12);
 std::vector<std::uint64_t> seen;
 auto n = clock.advance(1.0, [&](double dt, std::uint64_t i) {
  REQUIRE(dt == clock.step());
  seen.push_back(i);
 });
 REQUIRE(n == 5);
 REQUIRE(clock.tick_index() == 5);
 REQUIRE(seen == std::vector<std::uint64_t>{0, 1, 2, 3, 4});
 REQUIRE(clock.alpha() >= 0.0);
 REQUIRE(clock.alpha() < 1.0);
}

TEST_CASE("fixed tick produces identical tick sequence for bounded frame slices") {
 nuc::sim::FixedTick one, sliced;
 std::vector<std::uint64_t> a, b;
 one.advance(4.0 / 15.0, [&](double, std::uint64_t i) { a.push_back(i); });
 for (int i = 0; i < 4; ++i)
  sliced.advance(1.0 / 15.0, [&](double, std::uint64_t tick) { b.push_back(tick); });
 REQUIRE(a == b);
 REQUIRE(one.tick_index() == sliced.tick_index());
 REQUIRE(std::abs(one.alpha() - sliced.alpha()) < 1e-10);
}

TEST_CASE("xoshiro streams repeat, restore, and reject invalid state") {
 nuc::sim::Rng a(1234), b(1234);
 for (int i = 0; i < 100; ++i) REQUIRE(a.next() == b.next());
 auto t0 = nuc::sim::Rng::for_tick(42, 0), t1 = nuc::sim::Rng::for_tick(42, 1);
 REQUIRE(t0.next() != t1.next());
 auto snapshot = a.state();
 const auto expected = a.next();
 a.set_state(snapshot);
 REQUIRE(a.next() == expected);
 REQUIRE_THROWS(a.set_state({0, 0, 0, 0}));
 for (int i = 0; i < 100; ++i) {
  const auto u = a.unit();
  REQUIRE(u >= 0.0);
  REQUIRE(u < 1.0);
 }
}

TEST_CASE("state hash uses stable little endian integers") {
 nuc::sim::StateHash a, b;
 a.integer<std::uint32_t>(0x12345678U);
 b.integer<std::uint8_t>(0x78);
 b.integer<std::uint8_t>(0x56);
 b.integer<std::uint8_t>(0x34);
 b.integer<std::uint8_t>(0x12);
 REQUIRE(a.value() == b.value());
 a.text("world");
 REQUIRE(a.value() != b.value());
}

TEST_CASE("deterministic simulation matches after 1000 ticks") {
 struct State { std::int64_t position = 0; std::int64_t velocity = 0; std::uint64_t random_sum = 0; };
 const auto run = [] {
  State state;
  nuc::sim::StateHash hash;
  constexpr std::uint64_t seed = 0xC0FFEEULL;
  for (std::uint64_t tick = 0; tick < 1000; ++tick) {
   auto rng = nuc::sim::Rng::for_tick(seed, tick, 7);
   state.velocity += static_cast<std::int64_t>(rng.next() % 7) - 3;
   state.position += state.velocity;
   state.random_sum += rng.next() & 0xffffU;
   hash.integer(tick);
   hash.integer(state.position);
   hash.integer(state.velocity);
   hash.integer(state.random_sum);
  }
  return std::pair{state, hash.value()};
 };
 const auto first = run();
 const auto second = run();
 REQUIRE(first.first.position == second.first.position);
 REQUIRE(first.first.velocity == second.first.velocity);
 REQUIRE(first.first.random_sum == second.first.random_sum);
 REQUIRE(first.second == second.second);
}
