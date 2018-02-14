#include "QuadTree.hpp"
#include "gtest/gtest.h"
#include <vector>
#include <array>
#include <algorithm>
#include <memory>

struct Vec3Test
{
    float x, y, z;
};

using Obj = QT::Object<Vec3Test>;
using TreeType = QT::Tree<Obj>;
class QuadTreeFixture : public ::testing::Test
{
public:
    TreeType testTree;
    std::vector<Obj> objVector;

    QuadTreeFixture()
    {
        auto radius = 1000.0;
        testTree = TreeType(radius);
        objVector.resize(1000);
    }

};

TEST_F(QuadTreeFixture, insert)
{
    auto testRect =  QT::Rect(0.0, 0.0, 100.0);
    objVector[0].boundingBox = testRect;
    testTree.InsertOrUpdate(&objVector[0]);
    auto iter = testTree.GetIteratorForRegion(objVector[0].boundingBox);
    auto foundRect = (*iter).boundingBox;
    EXPECT_EQ(testRect, foundRect);
    auto counter = 0U;
    while (!iter.PastEnd())
    {
        ++counter;
        ++iter;
    }
    EXPECT_EQ(1U, counter);
}

TEST_F(QuadTreeFixture, insert_duplicates)
{
    objVector[0].boundingBox = QT::Rect(0.0, 0.0, 100.0);
    testTree.InsertOrUpdate(&objVector[0]);
    testTree.InsertOrUpdate(&objVector[0]);
    testTree.InsertOrUpdate(&objVector[0]);
    auto iter = testTree.GetIteratorForRegion(objVector[0].boundingBox);
    auto counter = 0U;
    while (!iter.PastEnd())
    {
        ++counter;
        ++iter;
    }
    EXPECT_EQ(1U, counter);
}

int main(int argc, char **argv) 
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}