#include <catch2/catch_test_macros.hpp>
#include "core/jobs.h"
#include <vector>
TEST_CASE("parallel_for_sums_correctly"){nuc::Jobs jobs(2);std::vector<int> v(1000);jobs.parallel_for(v.size(),[&](std::size_t i){v[i]=static_cast<int>(i);});long long sum=0;for(auto x:v)sum+=x;REQUIRE(sum==499500);}
