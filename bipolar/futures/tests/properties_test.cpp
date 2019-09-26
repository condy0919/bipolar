#include "bipolar/futures/properties.hpp"

#include <gtest/gtest.h>

using namespace bipolar;

struct FooCategory {};
struct BarCategory {};

struct FooProperty : Property<FooCategory> {};
struct BarProperty : Property<BarCategory> {};

struct DuckProperty {
    using property_category = FooCategory;
};

TEST(Properties, property) {
    EXPECT_TRUE(is_property_v<FooProperty>);
    EXPECT_FALSE(is_property_v<int>);
    EXPECT_TRUE(is_property_v<DuckProperty>);
}

TEST(Properties, property_set) {
    using PS = PropertySet<FooProperty>;
    EXPECT_TRUE(is_property_set_v<PS>);
    EXPECT_FALSE(is_property_set_v<DuckProperty>);
}

TEST(Properties, property_query) {
    using PS0 = PropertySet<FooProperty>;
    EXPECT_TRUE((property_query_v<PS0, FooProperty>));
    EXPECT_FALSE((property_query_v<PS0, BarProperty>));
    EXPECT_FALSE((property_query_v<int, FooProperty>));
    EXPECT_FALSE((property_query_v<PS0, int>));
    EXPECT_FALSE((property_query_v<PS0, FooProperty, BarProperty>));

    struct BazProperty : FooProperty {};
    using PS1 = PropertySet<BarProperty, BazProperty>;
    EXPECT_TRUE((property_query_v<PS1, FooProperty>));
    EXPECT_TRUE((property_query_v<PS1, BarProperty>));
    EXPECT_TRUE((property_query_v<PS1, FooProperty, BarProperty>));
    EXPECT_TRUE((property_query_v<PS1, BazProperty, BarProperty>));
}

TEST(Properties, category_query) {
    using PS0 = PropertySet<FooProperty, BarProperty>;
    EXPECT_FALSE((category_query_v<PS0, int>));
    EXPECT_FALSE((category_query_v<PS0, FooProperty>));
    EXPECT_TRUE((category_query_v<PS0, FooCategory>));

    struct BazProperty : FooProperty {};
    using PS1 = PropertySet<BarProperty, BazProperty>;
    EXPECT_TRUE((category_query_v<PS1, BarCategory>));
    EXPECT_TRUE((category_query_v<PS1, FooCategory>));
    EXPECT_FALSE((category_query_v<PS1, BazProperty>));
    EXPECT_FALSE((category_query_v<PS1, FooProperty>));
}
