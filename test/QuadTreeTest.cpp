#include "QuadTree.hpp"
#include "gtest/gtest.h"
#include <vector>
#include <array>
#include <algorithm>
#include <memory>
#include <chrono>

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
        objVector.resize(10000);
        auto startTime = std::chrono::high_resolution_clock::now();
        for (auto& obj : objVector)
        {
            auto time = std::chrono::high_resolution_clock::now();
            auto pos = static_cast<double>((time - startTime).count() % static_cast<uint64_t>(radius * 2.0));
            pos -= radius;
            obj.boundingBox = QT::Rect(pos, pos, 50.0);
        }
    }

};

TEST_F(QuadTreeFixture, insert)
{
    auto testRect =  QT::Rect(0.0, 0.0, 100.0);
    objVector[0].boundingBox = testRect;
    testTree.InsertOrUpdate(&objVector[0]);
    auto iter = testTree.GetIteratorForRegion(objVector[0].boundingBox);
    auto foundRect = iter->boundingBox;
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

// success depends on choosing a small enough retrieval region to not request the entire tree
// the bit math to select the retrieval area rounds up, causing unexpected results sometimes
TEST_F(QuadTreeFixture, insert_retrieve_partial)
{
    // top left and top right depth 1 quadrants
    objVector[0].boundingBox = QT::Rect(-500.0, -500.0, 100.0);
    objVector[1].boundingBox = QT::Rect(500.0, -500.0, 100.0);
    testTree.InsertOrUpdate(&objVector[0]);
    testTree.InsertOrUpdate(&objVector[1]);
    auto iter = testTree.GetIteratorForRegion(QT::Rect(-500.0, -500.0, 400.0));
    EXPECT_EQ(QT::Rect(-500.0, -500.0, 100.0), iter->boundingBox);
    auto counter = 0U;
    while (!iter.PastEnd())
    {
        ++counter;
        ++iter;
    }
    EXPECT_EQ(1U, counter);
}

TEST_F(QuadTreeFixture, insert_performance)
{
    for (auto& obj : objVector)
    {
        testTree.InsertOrUpdate(&obj);
    }
}

TEST_F(QuadTreeFixture, touch_all_objects)
{
    for (auto& obj : objVector)
    {
        obj.data.x = 1.0f;
    }
}

int main(int argc, char **argv) 
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}