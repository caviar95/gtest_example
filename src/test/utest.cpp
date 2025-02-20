#include "../test_factorial.h"

#include <limits.h>

#include "gtest/gtest.h"

namespace {

TEST(FactorialTest, Negative)
{
    EXPECT_EQ(1, Factorial(-5));
    EXPECT_EQ(1, Factorial(-1));
}

}
