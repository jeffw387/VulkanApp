#include <array>
#include <list>
#include "IntrusiveList.hpp"
#include "gtest/gtest.h"

struct TestNode {
  TestNode* next = nullptr;
  TestNode* previous = nullptr;
  int x;
};

struct TestNodeLight {
  int x;
};

class IntrusiveListFixture : public ::testing::Test {
 public:
  IntrusiveList<TestNode> testList;
  std::array<TestNode, 10> testNodes;
  virtual void SetUp() {
    int i = 0;
    for (auto& node : testNodes) {
      node.x = i;
      ++i;
    }
  }
};

class IntrusiveListPerformanceFixture : public ::testing::Test {
 public:
  std::array<TestNode, 5000> testNodes;
  std::array<TestNodeLight, 5000> testNodesLight;
  std::list<TestNodeLight> testStdList;
  IntrusiveList<TestNode> testIntrusiveList;

  virtual void SetUp() {
    int i = 0;
    for (auto& node : testNodes) {
      node.x = i;
      ++i;
    }
    i = 0;
    for (auto& node : testNodesLight) {
      node.x = i;
      ++i;
    }
  }
};

TEST_F(IntrusiveListFixture, push_front) {
  testList.push_front(&testNodes[0]);
  testList.push_front(&testNodes[1]);

  EXPECT_EQ(1, testList.front()->x);
  EXPECT_EQ(0, testList.front()->next->x);
  EXPECT_EQ(2, testList.size());
}

TEST_F(IntrusiveListFixture, pop_front) {
  testList.push_front(&testNodes[0]);
  testList.pop_front();
  EXPECT_EQ(nullptr, testList.front());
  EXPECT_EQ(0, testList.size());
}

// this is not intended behavior but I don't see an easy solution
TEST_F(IntrusiveListFixture, add_duplicates) {
  testList.push_front(&testNodes[0]);
  testList.push_front(&testNodes[0]);
  EXPECT_EQ(2U, testList.size());
  testList.pop_front();
  testList.pop_front();
  EXPECT_EQ(0U, testList.size());
}

TEST_F(IntrusiveListFixture, erase_root) {
  testList.push_front(&testNodes[0]);
  EXPECT_EQ(1U, testList.size());
  testList.erase(&testNodes[0]);
}

TEST_F(IntrusiveListFixture, iterate) {
  for (auto& node : testNodes) {
    testList.push_front(&node);
  }
  EXPECT_EQ(10U, testList.size());

  TestNode* nodePtr = testList.front();
  while (nodePtr->next != nullptr) {
    nodePtr = nodePtr->next;
  }
  EXPECT_EQ(nullptr, nodePtr->next);
  for (auto i = 0U; i < 10U; ++i) {
    EXPECT_EQ(i, nodePtr->x);
    if (nodePtr->previous != nullptr) nodePtr = nodePtr->previous;
  }
  EXPECT_EQ(9, nodePtr->x);
  EXPECT_EQ(nullptr, nodePtr->previous);
}

TEST_F(IntrusiveListPerformanceFixture, std_list_performance) {
  for (auto& node : testNodesLight) {
    testStdList.push_front(node);
  }
  auto begin = testStdList.begin();
  while (begin != testStdList.end()) {
    begin = testStdList.erase(begin);
  }
}

TEST_F(IntrusiveListPerformanceFixture, intrusive_list_performance) {
  for (auto& node : testNodes) {
    testIntrusiveList.push_front(&node);
  }
  auto begin = testIntrusiveList.front();
  while (begin != nullptr) {
    testIntrusiveList.pop_front();
    begin = testIntrusiveList.front();
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}