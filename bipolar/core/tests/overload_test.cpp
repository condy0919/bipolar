#include "bipolar/core/overload.hpp"

#include <vector>
#include <variant>

#include <gtest/gtest.h>

using namespace bipolar;
using namespace std::literals;

TEST(Overload, overload) {
    std::vector<std::variant<int, const char*>> vvs = {"hello", 42};
    for (const auto& v : vvs) {
        std::visit(Overload{
                       [](const char* s) { EXPECT_EQ(s, "hello"s); },
                       [](int x) { EXPECT_EQ(x, 42); },
                   },
                   v);
    }
}
