#include <catch2/catch_test_macros.hpp>
#include "app/edit_queue.h"
#include <vector>

using nuc::EditAction;

TEST_CASE("a press between ticks is queued and drained inside the next tick") {
    nuc::EditRequestQueue queue;
    // A frame with the button down, then a frame with it released: exactly one edit.
    queue.observe(true,false);
    queue.observe(false,false);
    REQUIRE(queue.pending()==1);
    const auto drained=queue.drain();
    REQUIRE(drained.size()==1);
    REQUIRE(drained.front()==EditAction::break_block);
    REQUIRE(queue.pending()==0);
}
TEST_CASE("holding a button does not repeat the edit every frame") {
    nuc::EditRequestQueue queue;
    for(int frame=0;frame<10;++frame) queue.observe(true,true);
    REQUIRE(queue.drain().size()==2);
    REQUIRE(queue.drain().empty());
}
TEST_CASE("edits reach the tick in arrival order regardless of frame rate") {
    // The same two clicks, split across very different frame counts, must produce the
    // same per-tick edit sequence: this is the frame-rate independence networking needs.
    const auto run_tick=[](int frames_before_first_click,int frames_before_second_click){
        nuc::EditRequestQueue queue;
        std::vector<EditAction> applied;
        auto frame=[&](bool left,bool right){queue.observe(left,right);};
        for(int i=0;i<frames_before_first_click;++i) frame(false,false);
        frame(true,false);
        frame(false,false);
        for(int i=1;i<frames_before_second_click;++i) frame(false,false);
        frame(false,true);
        frame(false,false);
        for(const auto action:queue.drain()) applied.push_back(action);
        return applied;
    };
    const auto slow=run_tick(2,6);
    const auto fast=run_tick(17,31);
    REQUIRE(slow.size()==2);
    REQUIRE(slow==fast);
    REQUIRE(slow[0]==EditAction::break_block);
    REQUIRE(slow[1]==EditAction::place_block);
}
TEST_CASE("the queue is bounded so a stuck button cannot grow it without limit") {
    nuc::EditRequestQueue queue;
    for(std::size_t i=0;i<nuc::EditRequestQueue::max_pending+10;++i) {
        queue.observe(true,false);
        queue.observe(false,false);
    }
    REQUIRE(queue.pending()==nuc::EditRequestQueue::max_pending);
    queue.reset();
    REQUIRE(queue.pending()==0);
}
