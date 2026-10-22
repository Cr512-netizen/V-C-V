#include <catch2/catch_test_macros.hpp>
#include "sim/energy.h"
#include "sim/fields.h"
#include "sim/fluids.h"
#include <cmath>
TEST_CASE("energy network balances production and priority consumption") {
 nuc::sim::EnergyNetwork net;net.add({1,nuc::sim::EnergyRole::Producer,10});net.add({2,nuc::sim::EnergyRole::Consumer,0,7,0,0,0,5});net.add({3,nuc::sim::EnergyRole::Consumer,0,7,0,0,0,1});net.add({4,nuc::sim::EnergyRole::Storage,0,0,20,0});net.tick();REQUIRE(net.find(2)->allocated==7);REQUIRE(net.find(3)->allocated==3);REQUIRE(net.find(4)->stored==0);REQUIRE(net.produced_last_tick()==10);REQUIRE(net.consumed_last_tick()==10);
}
TEST_CASE("fluid diffusion preserves volume and responds to pressure") {
 nuc::sim::FluidGrid grid(3,1,1,0.2);grid.set(0,0,0,{1,1.0,1.0});const auto before=grid.total_volume();grid.tick();REQUIRE(grid.at(0,0,0).volume<1.0);REQUIRE(grid.at(1,0,0).volume>0.0);REQUIRE(std::abs(grid.total_volume()-before)<1e-9);REQUIRE(grid.at(1,0,0).pressure()>0.0);
}
TEST_CASE("field spreads from source and decays without further input") {
 nuc::sim::FieldGrid field(5,1,1,0.15,0.01);field.set(2,0,0,10);field.tick();const auto center=field.at(2,0,0);const auto first_total=field.total();REQUIRE(center>0);REQUIRE(field.at(1,0,0)>0);field.tick();REQUIRE(field.total()<first_total);
}
