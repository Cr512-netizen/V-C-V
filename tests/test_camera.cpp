#include <catch2/catch_test_macros.hpp>
#include "platform/camera.h"
#include <cmath>

namespace {
bool finite_matrix(const glm::mat4& m) {
    for (int c=0;c<4;++c) for (int r=0;r<4;++r)
        if (!std::isfinite(m[c][r])) return false;
    return true;
}
}
TEST_CASE("camera view projection starts finite") {
    const nuc::Camera camera;
    REQUIRE(finite_matrix(camera.view_projection()));
}
TEST_CASE("camera aspect ratio changes projection") {
    nuc::Camera camera;
    const auto before=camera.projection();
    camera.set_aspect(1.0F);
    const auto after=camera.projection();
    REQUIRE(before[0][0] != after[0][0]);
    REQUIRE(finite_matrix(after));
}
TEST_CASE("camera yaw changes forward direction") {
    nuc::Camera camera;
    camera.set_rotation(-90.0F,0.0F);
    const auto before=camera.forward();
    camera.set_rotation(0.0F,0.0F);
    const auto after=camera.forward();
    REQUIRE(std::abs(before.x-after.x)>0.9F);
}
TEST_CASE("camera pitch clamps before the view becomes singular") {
    nuc::Camera camera;
    camera.set_rotation(0.0F,120.0F);
    REQUIRE(camera.forward().y < 1.0F);
    REQUIRE(camera.forward().y > 0.99F);
    REQUIRE(finite_matrix(camera.view()));
}
TEST_CASE("camera update with null window or invalid delta leaves position unchanged") {
    nuc::Camera camera;
    const auto before=camera.position();
    camera.move_local({0,0,1},0.0);
    camera.move_local({0,0,1},-1.0);
    REQUIRE(glm::distance(camera.position(),before)<0.000001F);
    camera.move_local({0,0,1},0.1);
    REQUIRE(std::abs(glm::distance(camera.position(),before)-0.9F)<0.001F);
}
