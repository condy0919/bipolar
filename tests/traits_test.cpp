#include "bipolar/core.hpp"
#include <gtest/gtest.h>

using namespace bipolar;

struct NoEquality {};
static_assert(!is_equality_comparable_v<NoEquality>);

struct IncompleteEquality {};
bool operator==(const IncompleteEquality&, const IncompleteEquality&);
static_assert(!is_equality_comparable_v<IncompleteEquality>);

struct IncompleteEquality2 {};
int operator==(const IncompleteEquality2&, const IncompleteEquality2&);
bool operator!=(const IncompleteEquality2&, const IncompleteEquality2&);
static_assert(!is_equality_comparable_v<IncompleteEquality2>);

struct Equality {};
bool operator==(const Equality&, const Equality&);
bool operator!=(const Equality&, const Equality&);
static_assert(is_equality_comparable_v<Equality>);

struct LessThan {};
bool operator<(const LessThan&, const LessThan&);
static_assert(is_less_than_comparable_v<LessThan>);

struct LessThanOrEqualTo {};
bool operator<=(const LessThanOrEqualTo&, const LessThanOrEqualTo&);
static_assert(is_less_than_or_equal_to_comparable_v<LessThanOrEqualTo>);

struct GreaterThan {};
bool operator>(const GreaterThan&, const GreaterThan&);
static_assert(is_greater_than_comparable_v<GreaterThan>);

struct GreaterThanOrEqualTo {};
bool operator>=(const GreaterThanOrEqualTo&, const GreaterThanOrEqualTo&);
static_assert(is_greater_than_or_equal_to_comparable_v<GreaterThanOrEqualTo>);

struct TotallyOrdered {};
bool operator==(const TotallyOrdered&, const TotallyOrdered&);
bool operator!=(const TotallyOrdered&, const TotallyOrdered&);
bool operator<(const TotallyOrdered&, const TotallyOrdered&);
bool operator<=(const TotallyOrdered&, const TotallyOrdered&);
bool operator>(const TotallyOrdered&, const TotallyOrdered&);
bool operator>=(const TotallyOrdered&, const TotallyOrdered&);
static_assert(is_strict_totally_ordered_v<TotallyOrdered>);
