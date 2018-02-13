#include "gtest/gtest.h"
#include "IntrusiveList.hpp"
#include <array>

class IntrusiveListFixture : public ::testing::Test
{
public:
    IntrusiveForwardList<TestNode> testList;
    std::array<TestNode, 10> testNodes;
    IntrusiveListFixture()
    {
        int i = 0;
        for (auto node : testNodes)
        {
            node.x = i;
            ++i;
        }
    }
}

TEST_F(IntrusiveListFixture, push_front_test)
{
    testList.push_front(&testNodes[0]);
    testList.push_front(&testNodes[1]);

    EXPECT_EQ(1, testList.front()->x);
    EXPECT_EQ(0, testList.front()->next->x);
    EXPECT_EQ(2, testList.size());
}

int main(int argc, char **argv) 
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}