#include <catch2/catch_test_macros.hpp>
#include "render/null_renderer.h"
TEST_CASE("renderer_counts_frames"){nuc::NullRenderer r;REQUIRE(r.init());for(int i=0;i<8;++i){r.begin_frame();r.end_frame();}REQUIRE(r.frames()==8);r.shutdown();}
