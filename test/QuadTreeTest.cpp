#include "QuadTree.hpp"
#include "gtest/gtest.h"
#include <vector>
#include <array>
#include <iterator>
#include <algorithm>


struct Vec3Test
{
    float x, y, z;
};

class QuadTreeFixture : public ::testing::Test
{
public:
    QT::Tree<Vec3Test> testTree;
    std::array<QT::Tree<Vec3Test>::Object, 1000> objectArray;

    virtual void SetUp()
    {
        auto radius = 1000.0;
        testTree = QT::Tree<Vec3Test>(radius);
    }

};

TEST_F(QuadTreeFixture, insert_test)
{
    testTree.InsertOrUpdate(&objectArray[0]);
}

int main(int argc, char **argv) 
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}