#include <catch2/catch_test_macros.hpp>
#include "script/events.h"
TEST_CASE("typed voxel break events preserve coordinates and material") {
 nuc::script::EventBus bus; nuc::script::Event received; bool called=false;
 bus.subscribe(nuc::script::EventType::OnVoxelBreak,[&](const auto& event){received=event;called=true;});
 nuc::script::Event event; event.type=nuc::script::EventType::OnVoxelBreak;
 event.x=-4;event.y=12;event.z=8;event.material=37;
 bus.emit(event);
 REQUIRE(called);REQUIRE(received.x==-4);REQUIRE(received.y==12);REQUIRE(received.z==8);REQUIRE(received.material==37);
}
TEST_CASE("event subscriptions can be removed and do not see other event types") {
 nuc::script::EventBus bus;int calls=0;
 auto token=bus.subscribe(nuc::script::EventType::OnSave,[&](const auto&){++calls;});
 nuc::script::Event tick;tick.type=nuc::script::EventType::OnTick;bus.emit(tick);REQUIRE(calls==0);
 nuc::script::Event save;save.type=nuc::script::EventType::OnSave;bus.emit(save);REQUIRE(calls==1);
 REQUIRE(bus.unsubscribe(token));bus.emit(save);REQUIRE(calls==1);
}
