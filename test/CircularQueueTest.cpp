#include "gtest/gtest.h"
#include "../CircularQueue.hpp"

#include <iostream>

class QueueTestFixture : public ::testing::Test
{
public:
    CircularQueue<int, 5> queue;
};

TEST_F(QueueTestFixture, DefaultQueueChecks)
{
    ASSERT_TRUE(queue.size() == 0);
    ASSERT_TRUE(queue.capacity() == 5);
}

TEST_F(QueueTestFixture, QueueEmptyCheck)
{
    ASSERT_TRUE(queue.size() == 0);
    auto option = queue.readFirst();
    ASSERT_FALSE(option.has_value());
}

TEST_F(QueueTestFixture, QueueAddLastElement)
{
    const int testInt = 2;
    queue.pushLast(testInt);
    ASSERT_TRUE(queue.size() == 1);
    auto first = queue.readFirst();
    ASSERT_TRUE(queue.size() == 1);
    ASSERT_TRUE(first.has_value());
    std::cout << "First value: " << first.value() << "\n";
    std::cout << "Test value: " << testInt << "\n";
    ASSERT_TRUE(first.value() == testInt);
}

TEST_F(QueueTestFixture, QueueRemoveFirstElement)
{
    const int testInt = 3;
    queue.pushLast(testInt);
    auto first = queue.popFirst();
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(first.value() == testInt);
    ASSERT_TRUE(queue.size() == 0);
}

TEST_F(QueueTestFixture, QueueOverfill)
{
    for (auto i = 0; i < 5; i++)
    {
        ASSERT_TRUE(queue.pushLast(i));
    }
    ASSERT_FALSE(queue.pushLast(5));
}

TEST_F(QueueTestFixture, QueueWrapAround)
{
    for (auto i = 0; i < 5; i++)
    {
        ASSERT_TRUE(queue.pushLast(i));
    }
    auto first = queue.popFirst();
    ASSERT_TRUE(queue.pushLast(5));
}

int main(int argc, char **argv) 
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}