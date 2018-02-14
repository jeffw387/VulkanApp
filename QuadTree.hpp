#pragma once
#include <array>
#include <vector>
#include <algorithm>
#include <memory>
#include "IntrusiveList.hpp"

namespace QT
{
    struct Rect
    {
        Rect() noexcept = default;
        Rect(Rect&&) noexcept = default;
        Rect& operator=(Rect&&) noexcept = default;
        Rect(const Rect&) noexcept = default;
        Rect& operator=(const Rect&) noexcept = default;

        Rect(double x, double y, double radius) :
            x(x), y(y), xRadius(radius), yRadius(radius)
        {}

        Rect(double x, double y, double xRadius, double yRadius) :
            x(x), y(y), xRadius(xRadius), yRadius(yRadius)
        {}

        bool ContainsPoint(const double x, const double y) const
        {
            return  (x >= GetLeft()) && 
                    (x <= GetRight()) && 
                    (y >= GetTop()) && 
                    (y <= GetBottom());
        }

        bool ContainsRect(const Rect& rect) const
        {
            return  (rect.GetLeft()   >= GetLeft())  && 
                    (rect.GetRight()  <= GetRight()) && 
                    (rect.GetTop()    >= GetTop())   && 
                    (rect.GetBottom() <= GetBottom());
        }

        double GetLeft() const
        {
            return x - xRadius;
        }

        double GetRight() const
        {
            return x + xRadius;
        }

        double GetTop() const
        {
            return y - yRadius;
        }

        double GetBottom() const
        {
            return y + yRadius;
        }

        friend bool operator==(const Rect& lhs, const Rect& rhs)
        {
            return  (lhs.x == rhs.x) &&
                    (lhs.y == rhs.y) &&
                    (lhs.xRadius == rhs.xRadius) &&
                    (lhs.yRadius == rhs.yRadius);
        }
    private:
        double x, y, xRadius, yRadius;
    };


    template <typename T>
    struct Object
    {
        T data;
        Rect boundingBox;
        Object* next;
        Object* previous;
        size_t nodeIndex;
    };

    template <typename ObjectType>
    class Tree
    {
    public:
        static constexpr size_t TotalNodes = 1 + (2*2) + (4*4) + (8*8) + (16*16) + (32*32) + (64*64) + (128*128);

        struct Node
        {
        public:
            friend class Tree;
            Node() noexcept = default;

            // copy from rect constructor
            explicit Node(const Rect& region) : rect(region)
            {}

            // move assign a rect
            Node& operator=(Rect&& region)
            {
                rect = std::move(region);
            }

            ObjectType* begin()
            {
                return objects.front();
            }

            void InsertObject(ObjectType* objectPtr)
            {
                objects.push_front(objectPtr);
            }

            void RemoveObject(ObjectType* objectPtr)
            {
                objects.erase(objectPtr);
            }

            void SetRegion(const Rect& region)
            {
                rect = region;
            }

            void SetRegion(Rect&& region)
            {
                rect = std::move(region);
            }

            const Rect& GetRegion()
            {
                return rect;
            }
            std::array<Node*, 4> children;

            size_t size()
            {
                return objects.size();
            }
        private:
            IntrusiveList<ObjectType> objects;
            Rect rect;
        };

        class RegionIterator
        {
        public:
            friend class Tree;
            RegionIterator() noexcept = default;

            RegionIterator(std::vector<Node*>&& nodePtrsTemp) : 
                nodePtrs(std::move(nodePtrsTemp))
            {
                if (nodePtrs.size() > 0)
                {
                    current = nodePtrs[currentNode]->begin();
                }
            }

            ObjectType& operator*()
            {
                return *current;
            }

            ObjectType& operator->()
            {
                return *current;
            }

            RegionIterator& operator++()
            {
                if (current->next == nullptr)
                {
                    if (++currentNode >= nodePtrs.size())
                    {
                        current = nullptr;
                        return *this;
                    }
                    current = nodePtrs[currentNode]->begin();
                    return *this;
                }
                current = current->next;
                return *this;
            }

            bool PastEnd()
            {
                return current == nullptr;
            }

        private:
            std::vector<Node*> nodePtrs;
            size_t currentNode = 0U;
            ObjectType* current = nullptr;
        };

        Tree& operator=(const Tree& tree) = default;
        Tree& operator=(Tree&& tree) = default;

        Tree() noexcept = default;

        // radius must be positive
        Tree(const double& radius) : 
            radius(radius),
            inverseRadius(255.0/(radius * 2))
        {
            nodes.resize(TotalNodes);
            auto radiusAtDepth = radius;
            auto nodeIndex = 0U;
            for (auto depth = 0U; depth < 7U; ++depth)
            {
                auto cellsPerAxis = 1U << depth;
                for (auto y = 0U; y < cellsPerAxis; y++)
                {
                    for (auto x = 0U; x < cellsPerAxis; ++x)
                    {
                        Node& node = nodes[nodeIndex];
                        node.SetRegion(Rect(
                                (2 * x * radiusAtDepth) - radius + radiusAtDepth, 
                                (2 * y * radiusAtDepth) - radius + radiusAtDepth, 
                                radiusAtDepth));
                        // if we're not at the root, we have a parent node to find
                        if (depth)
                        {
                            auto parentX = x >> 1U;
                            auto parentY = y >> 1U;
                            Node& parent = nodes[GetIndexAt(depth - 1, parentX, parentY)];
                            uint8_t childX = x & 1U;
                            uint8_t childY = y & 1U;
                            parent.children[(childY << 1U) + childX] = &node;
                        }
                        else
                        {
                            root = &node;
                        }
                        ++nodeIndex;
                    }
                }
                radiusAtDepth *= 0.5;
            }
        }

        void InsertOrUpdate(ObjectType* objectPtr)
        {
            // find new position in quadtree
            // if it is unchanged from old, return
            auto newIndex = ChooseIndex(objectPtr->boundingBox);
            auto oldIndex = objectPtr->nodeIndex;
            nodes[oldIndex].RemoveObject(objectPtr);
            nodes[newIndex].InsertObject(objectPtr);
        }

        void Remove(ObjectType* objectPtr)
        {
            nodes[objectPtr->nodeIndex].RemoveObject(objectPtr);
        }

        void RecurseTree(std::vector<Node*>& nodePtrs, Node* node)
        {
            if (node == nullptr)
                    return;
                if (node->size() > 0)
                {
                    nodePtrs.push_back(node);
                }
                for (Node* child : node->children)
                {
                    RecurseTree(nodePtrs, child);
                }
                return;
        }

        RegionIterator GetIteratorForRegion(const Rect& region)
        {
            auto regionIndex = ChooseIndex(region);

            std::vector<Node*> nodePtrsTemp;
            RecurseTree(nodePtrsTemp, &nodes[regionIndex]);
            
            RegionIterator iter = RegionIterator(std::move(nodePtrsTemp));
            return iter;
        }

    private:
        static uint8_t HighestBit(const uint8_t& bits)
        {
            static constexpr uint8_t Bit7 = 1 << 7;
            static constexpr uint8_t Bit6 = 1 << 6;
            static constexpr uint8_t Bit5 = 1 << 5;
            static constexpr uint8_t Bit4 = 1 << 4;
            static constexpr uint8_t Bit3 = 1 << 3;
            static constexpr uint8_t Bit2 = 1 << 2;
            static constexpr uint8_t Bit1 = 1 << 1;
            static constexpr uint8_t Bit0 = 1 << 0;

            // iterate through bits from highest to lowest
            if (bits & Bit7)
                return static_cast<uint8_t>(0);
            if (bits & Bit6)
                return static_cast<uint8_t>(1);
            if (bits & Bit5)                
                return static_cast<uint8_t>(2);
            if (bits & Bit4)
                return static_cast<uint8_t>(3);
            if (bits & Bit3)
                return static_cast<uint8_t>(4);
            if (bits & Bit2)
                return static_cast<uint8_t>(5);
            if (bits & Bit1)
                return static_cast<uint8_t>(6);
            return static_cast<uint8_t>(7);
        }

        size_t ChooseIndex(const Rect& rect)
        {
            if (!root->rect.ContainsRect(rect))
            {
                return 0;
            }

            // shift object rect coords into range [0, 255]
            auto left = rect.GetLeft();
            auto right = rect.GetRight();
            auto top = rect.GetTop();
            auto bottom = rect.GetBottom();
            auto leftScaled = (left + radius) * inverseRadius;
            auto rightScaled = (right + radius) * inverseRadius;
            auto topScaled = (top + radius) * inverseRadius;
            auto bottomScaled = (bottom + radius) * inverseRadius;
            auto xMin = static_cast<uint8_t>(std::clamp(std::floor(leftScaled),  0.0, 255.0));
            auto xMax = static_cast<uint8_t>(std::clamp(std::ceil(rightScaled),  0.0, 255.0));
            auto yMin = static_cast<uint8_t>(std::clamp(std::floor(topScaled),   0.0, 255.0));
            auto yMax = static_cast<uint8_t>(std::clamp(std::ceil(bottomScaled), 0.0, 255.0));

            auto xBits = xMin ^ xMax;
            auto yBits = yMin ^ yMax;
            
            xBits = HighestBit(xBits);
            yBits = HighestBit(yBits);

            auto treeDepth = std::min(xBits, yBits);

            xBits = xMin >> (8U - treeDepth);
            yBits = yMin >> (8U - treeDepth);

            size_t index = GetIndexAt(treeDepth, xBits, yBits);
            return index;
        }

        size_t GetIndexAt(const uint8_t depth, const uint8_t x, const uint8_t y)
        {
            auto nodeIndex = 0U;
            for (auto currentDepth = 0U; currentDepth < depth; ++currentDepth)
            {
                nodeIndex += (1U << currentDepth) * (1U << currentDepth);
            }
            auto sideLength = 1U << depth;
            nodeIndex += (sideLength * y) + x;
            return nodeIndex;
        }

        Node* root = nullptr;
        double radius = 0.0;
        double inverseRadius = 0.0;
        std::vector<Node> nodes;
    };
}