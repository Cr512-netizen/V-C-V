#include <catch2/catch_test_macros.hpp>
#include "core/jobs.h"
#include <cstdint>
#include <vector>

TEST_CASE("parallel_for sums correctly") {
    nuc::Jobs jobs(2);
    std::vector<int> values(1000, 0);
    jobs.parallel_for(values.size(), [&values](std::size_t index) { values[index] = static_cast<int>(index); });
    std::int64_t sum = 0;
    for (const auto value : values) sum += value;
    REQUIRE(sum == 499500);
}

TEST_CASE("thread count is clamped to a usable range") {
    nuc::Jobs one(1);
    REQUIRE(one.thread_count() == 1);
    nuc::Jobs huge(1000);
    REQUIRE(huge.thread_count() <= 64);
    REQUIRE(huge.thread_count() >= 1);
    nuc::Jobs automatic(0);
    REQUIRE(automatic.thread_count() >= 1);
}
