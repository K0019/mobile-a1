#include <iostream>
#include <gtest/gtest.h>
#include "CITest.h"

// Test declarations
TEST(GtestTest, OtherVariable) {
    ASSERT_NEAR(1.0f, 0.99999999f, cTestEpsilon);
    ASSERT_EQ(9 + 10, 21);
}

int main(int argc, char** argv) {
    // Init GoogleTest
    testing::InitGoogleTest(&argc, argv);

    // Run the tests
    return RUN_ALL_TESTS();
}