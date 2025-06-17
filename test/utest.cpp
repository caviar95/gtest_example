#include "test_factorial.h"

#include <limits.h>

#include "gtest/gtest.h"

TEST(FactorialTest, Negative)
{
    EXPECT_EQ(1, Factorial(-5));
    EXPECT_EQ(1, Factorial(-1));
    EXPECT_EQ(1, Factorial(3));
}

namespace {



}
