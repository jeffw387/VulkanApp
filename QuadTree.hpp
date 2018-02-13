#pragma once
#include <array>
#include <vector>
#include <algorithm>
#include "IntrusiveList.hpp"

namespace QT
{
    struct Rect
    {
        Rect() noexcept = default;
        Rect(Rect&&) noexcept = default;
        Rect& operator=(Rect&&) noexcept = default;

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

        bool operator==(const Rect& rhs)
        {
            return  (this->x == rhs.x) &&
                    (this->y == rhs.y) &&
                    (this->xRadius == rhs.xRadius) &&
                    (this->yRadius == rhs.yRadius);
        }
    private:
        double x, y, xRadius, yRadius;
    };


    template <typename T>
    class Tree
    {
    public:
        static constexpr size_t TotalNodes = 1 + (2*2) + (4*4) + (8*8) + (16*16) + (32*32) + (64*64) + (128*128);

        struct Object
        {
            friend class Tree;
            T data;
            Rect boundingBox;
            Object* next;
            Object* previous;
        private:
            size_t nodeIndex;
        };

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

            Object* begin()
            {
                return objects.front();
            }

            void InsertObject(Object* objectPtr)
            {
                objects.push_front(objectPtr);
            }

            void RemoveObject(Object* objectPtr)
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
            IntrusiveList<Object> objects;
            Rect rect;
        };

        class RegionIterator
        {
        public:
            friend class Tree;
            RegionIterator() noexcept = default;

            RegionIterator(std::vector<Node*>&& quadPtrsTemp) : 
                quadPtrs(std::move(quadPtrsTemp))
            {
                if (quadPtrs.size() > 0)
                {
                    current = quadPtrs[currentQuad]->begin();
                }
            }

            Object& operator->()
            {
                return *current;
            }

            Object& operator*()
            {
                return *current;
            }

            RegionIterator& operator++()
            {
                if (current->next == nullptr)
                {
                    if (++currentQuad > quadPtrs.size())
                    {
                        current = nullptr;
                        return *this;
                    }
                    current = quadPtrs[currentQuad].begin();
                    return *this;
                }
                current = current->next;
                return *this;
            }

        private:
            std::vector<Node*> quadPtrs;
            size_t currentQuad = 0U;
            Object* current = nullptr;
        };

        Tree() noexcept = default;

        // radius must be positive
        Tree(const double& radius) : 
            radius(radius),
            inverseRadius(255.0/static_cast<double>(radius))
        {
            auto radiusAtDepth = radius;
            for (auto depth = 0U; depth < 7U; ++depth)
            {
                auto cellsPerAxis = 1U << depth;
                for (auto y = 0U; y < cellsPerAxis; y++)
                {
                    for (auto x = 0U; x < cellsPerAxis; ++x)
                    {
                        Node& node = nodes[GetIndexAt(depth, x, y)];
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
                            parent.children[(y << 1U) + x] = &node;
                        }
                        else
                        {
                            root = &node;
                        }
                    }
                }
                radiusAtDepth *= 0.5;
            }
        }

        void InsertOrUpdate(Object* objectPtr)
        {
            // find new position in quadtree
            // if it is unchanged from old, return
            auto newIndex = ChooseIndex(objectPtr->boundingBox);
            auto oldIndex = objectPtr->nodeIndex;
            nodes[oldIndex].RemoveObject(objectPtr);
            nodes[newIndex].InsertObject(objectPtr);
        }

        void Remove(Object* objectPtr)
        {
            nodes[objectPtr->nodeIndex].RemoveObject(objectPtr);
        }

        RegionIterator GetIteratorForRegion(const Rect& region)
        {
            auto regionIndex = ChooseIndex(region);
            RegionIterator iter;
            auto recurseTree = [&](Node* node)
            {
                if (node == nullptr)
                    return;
                if (node->size() > 0)
                {
                    iter.quadPtrs.push_back(node);
                }
                for (auto& child : node.children)
                {
                    recurseTree(child);
                }
                return;
            };
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
            auto xMin = static_cast<uint8_t>(std::clamp(std::floor((rect.GetLeft()   + radius) * inverseRadius), 0.0, 255.0));
            auto yMin = static_cast<uint8_t>(std::clamp(std::floor((rect.GetTop()    + radius) * inverseRadius), 0.0, 255.0));
            auto xMax = static_cast<uint8_t>(std::clamp(std::ceil( (rect.GetRight()  + radius) * inverseRadius), 0.0, 255.0));
            auto yMax = static_cast<uint8_t>(std::clamp(std::ceil( (rect.GetBottom() + radius) * inverseRadius), 0.0, 255.0));

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
            return (depth * (x * (1U << depth) + y));
        }

        Node* root;
        double radius = 0.0;
        double inverseRadius = 0.0;
        std::array<Node, TotalNodes> nodes;
    };
}