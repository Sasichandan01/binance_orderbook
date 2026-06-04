#include <gtest/gtest.h>
#include "order_book.h"

TEST(OrderBookTest, DummyTest)
{
    EXPECT_TRUE(true);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}