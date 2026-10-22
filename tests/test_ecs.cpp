#include <catch2/catch_test_macros.hpp>
#include "core/ecs/world.h"
#include "core/ecs/view.h"
#include "core/ecs/systems.h"
#include <vector>
#include "core/sim/state_hash.h"
#include "core/sim/rng.h"
struct EPos { int x = 0; };
struct EVel { int x = 0; };
namespace nuc::ecs {
template<> struct ComponentStateTraits<EPos> {
 static constexpr bool registered = true;
 static constexpr std::uint64_t id = 0x1001;
 static void append(sim::StateHash& h, const EPos& p) { h.integer(p.x); }
};
template<> struct ComponentStateTraits<EVel> {
 static constexpr bool registered = true;
 static constexpr std::uint64_t id = 0x1002;
 static void append(sim::StateHash& h, const EVel& v) { h.integer(v.x); }
};
}
TEST_CASE("ECS reuse increments generation") {
 nuc::ecs::World w;
 auto a = w.create();
 w.emplace<EPos>(a, EPos{7});
 w.destroy(a);
 REQUIRE_FALSE(w.alive(a));
 auto b = w.create();
 REQUIRE(nuc::ecs::entity_index(a) == nuc::ecs::entity_index(b));
 REQUIRE(nuc::ecs::entity_generation(a) != nuc::ecs::entity_generation(b));
 REQUIRE_THROWS(w.get<EPos>(a));
}
TEST_CASE("ECS removal and destruction clean component membership") {
 nuc::ecs::World w;
 const auto e = w.create();
 w.emplace<EPos>(e, EPos{11});
 w.emplace<EVel>(e, EVel{2});
 w.remove<EPos>(e);
 REQUIRE_FALSE(w.has<EPos>(e));
 REQUIRE(w.has<EVel>(e));
 w.destroy(e);
 REQUIRE_FALSE(w.alive(e));
 REQUIRE_FALSE(w.has<EVel>(e));
 REQUIRE(w.entity_count() == 0);
}
TEST_CASE("ECS swap-and-pop preserves moved component ownership") {
 nuc::ecs::World w;
 const auto a = w.create();
 const auto b = w.create();
 const auto c = w.create();
 w.emplace<EPos>(a, EPos{10});
 w.emplace<EPos>(b, EPos{20});
 w.emplace<EPos>(c, EPos{30});
 w.remove<EPos>(b);
 REQUIRE_FALSE(w.has<EPos>(b));
 REQUIRE(w.get<EPos>(a).x == 10);
 REQUIRE(w.get<EPos>(c).x == 30);
 REQUIRE(w.try_pool<EPos>()->size() == 2);
 w.emplace<EPos>(b, EPos{22});
 REQUIRE(w.get<EPos>(b).x == 22);
 REQUIRE(w.get<EPos>(c).x == 30);
}
TEST_CASE("ECS destroy removes dense membership without corrupting survivors") {
 nuc::ecs::World w;
 const auto a = w.create();
 const auto b = w.create();
 const auto c = w.create();
 w.emplace<EPos>(a, EPos{1});
 w.emplace<EPos>(b, EPos{2});
 w.emplace<EPos>(c, EPos{3});
 w.destroy(a);
 REQUIRE_FALSE(w.has<EPos>(a));
 REQUIRE(w.get<EPos>(b).x == 2);
 REQUIRE(w.get<EPos>(c).x == 3);
 int count = 0;
 nuc::ecs::view<EPos>(w).each([&](nuc::ecs::Entity, EPos&) { ++count; });
 REQUIRE(count == 2);
}
TEST_CASE("ECS view iterates dense matching components") {
 nuc::ecs::World w;
 for (int i = 0; i < 10000; ++i) {
  auto e = w.create();
  w.emplace<EPos>(e, EPos{i});
  if (i % 2 == 0) w.emplace<EVel>(e, EVel{2});
 }
 int n = 0;
 long sum = 0;
 nuc::ecs::view<EPos, EVel>(w).each([&](nuc::ecs::Entity, EPos& p, EVel& v) { ++n; sum += p.x + v.x; });
 REQUIRE(n == 5000);
 REQUIRE(sum > 0);
}
TEST_CASE("ECS systems tick in registration order") {
 struct Rec : nuc::ecs::ISystem {
  int id;
  std::vector<int>& out;
  Rec(int i, std::vector<int>& o) : id(i), out(o) {}
  void tick(double) override { out.push_back(id); }
 };
 std::vector<int> o;
 nuc::ecs::SystemRunner r;
 r.add<Rec>(1, o);
 r.add<Rec>(2, o);
 r.tick(1.0 / 15.0);
 REQUIRE(o == std::vector<int>{1, 2});
}

TEST_CASE("ECS canonical state hash ignores insertion and pool allocation order") {
 nuc::ecs::World a, b;
 const auto a0 = a.create(), a1 = a.create();
 const auto b0 = b.create(), b1 = b.create();
 a.emplace<EPos>(a0, EPos{10}); a.emplace<EVel>(a0, EVel{3});
 a.emplace<EPos>(a1, EPos{20});
 b.emplace<EVel>(b0, EVel{3});
 b.emplace<EPos>(b1, EPos{20}); b.emplace<EPos>(b0, EPos{10});
 REQUIRE(a.deterministic_hash() == b.deterministic_hash());
}
TEST_CASE("ECS canonical state hash changes when component state changes") {
 nuc::ecs::World a;
 const auto e = a.create();
 a.emplace<EPos>(e, EPos{10});
 const auto before = a.deterministic_hash();
 a.get<EPos>(e).x = 11;
 REQUIRE(a.deterministic_hash() != before);
}
TEST_CASE("ECS canonical state hash rejects unregistered component types") {
 struct Unregistered { int x = 1; };
 nuc::ecs::World w;
 const auto e = w.create();
 w.emplace<Unregistered>(e, Unregistered{});
 REQUIRE_THROWS(w.deterministic_hash());
}

TEST_CASE("ECS canonical state hash includes future entity allocator state") {
 nuc::ecs::World a, b;
 const auto a0 = a.create(), a1 = a.create();
 const auto b0 = b.create(), b1 = b.create();
 a.destroy(a0); a.destroy(a1);
 b.destroy(b1); b.destroy(b0);
 REQUIRE(a.entity_count() == 0);
 REQUIRE(b.entity_count() == 0);
 REQUIRE(a.deterministic_hash() != b.deterministic_hash());
}
TEST_CASE("ECS simulation stays deterministic for 1000 ticks") {
 const auto make_world = [](bool velocity_pool_first) {
  auto world = std::make_unique<nuc::ecs::World>();
  std::vector<nuc::ecs::Entity> entities;
  for (int i = 0; i < 32; ++i) entities.push_back(world->create());
  if (velocity_pool_first) {
   for (auto e : entities) world->emplace<EVel>(e, EVel{static_cast<int>(nuc::ecs::entity_index(e) % 3)});
   for (auto e : entities) world->emplace<EPos>(e, EPos{});
  } else {
   for (auto e : entities) world->emplace<EPos>(e, EPos{});
   for (auto e : entities) world->emplace<EVel>(e, EVel{static_cast<int>(nuc::ecs::entity_index(e) % 3)});
  }
  return world;
 };
 auto a = make_world(false);
 auto b = make_world(true);
 for (std::uint64_t tick = 0; tick < 1000; ++tick) {
  const auto advance = [tick](nuc::ecs::World& world) {
   nuc::ecs::view<EPos, EVel>(world).each([tick](nuc::ecs::Entity e, EPos& p, EVel& v) {
    auto rng = nuc::sim::Rng::for_tick(0xC0FFEEULL, tick, nuc::ecs::entity_index(e));
    v.x += static_cast<int>(rng.next() % 5) - 2;
    p.x += v.x;
   });
  };
  advance(*a);
  advance(*b);
  REQUIRE(a->deterministic_hash() == b->deterministic_hash());
 }
}

TEST_CASE("ECS restores exact entity IDs and keeps unused indices reusable") {
 nuc::ecs::World world;
 const auto restored_id=nuc::ecs::make_entity(5,7);
 REQUIRE(world.create_at(restored_id)==restored_id);
 REQUIRE(world.alive(restored_id));
 const auto next=world.create();
 REQUIRE(nuc::ecs::entity_index(next)==4);
 REQUIRE(world.entity_count()==2);
 REQUIRE_THROWS(world.create_at(restored_id));
}
