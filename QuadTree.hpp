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

        Rect(double left, double right, double top, double bottom) :
            left(left), right(right), top(top), bottom(bottom), width(right - left), height(bottom - top)
        {}

        Rect(double x, double y, double radius) :
            Rect(x - radius, x + radius, y - radius, y + radius)
        {}

        bool ContainsPoint(const Point& point)
        {
            return ContainsPoint(point.x, point.y);
        }

        bool ContainsPoint(const double x, const double y) const
        {
            return (x >= left) && (x <= right) && (y <= bottom) && (y >= top);
        }

        bool ContainsRect(const Rect& rect) const
        {
            return  (rect.left   >= this->left)  && 
                    (rect.right  <= this->right) && 
                    (rect.top    >= this->top)   && 
                    (rect.bottom <= this->bottom);
        }

        double GetWidth()
        {
            return width;
        }

        double GetHeight()
        {
            return height;
        }

        double left, right, top, bottom;

    private:
        double width, height;
    };

    bool operator==(const Rect& a, const Rect& b)
    {
        return (a.left == b.left) &&
            (a.right == b.right) &&
            (a.top == b.top) &&
            (a.bottom == b.bottom);
    }

    template <typename T>
    class Tree
    {
    public:
        static constexpr size_t TotalNodes = 1 + (2*2) + (4*4) + (8*8) + (16*16) + (32*32) + (64*64) + (128*128);

        struct Object
        {
            Rect boundingBox;
            T data;
            Node* parentNode;
            Object* next;
            Object* previous;
        };

        struct Node
        {
        public:
            Node() noexcept = default;
            Node(const Rect& region) : rect(region)
            {}

            Object* begin()
            {
                return objects.front();
            }

            void InsertObject(Object* objectPtr)
            {
                objects.push_front(objectPtr);
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
            std::array<Node*>, 4> children;
        private:
            IntrusiveList<Object> objects;
            Rect rect;
        };

        class RegionIterator
        {
        public:
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

        size_t GetIndexAt(const uint8_t depth, const uint8_t x, const uint8_t y)
        {
            return (depth * (x * (1U << treeDepth) + y));
        }

        Tree() noexcept = default;
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
                        node.SetRegion(Rect(x * radiusAtDepth, y * radiusAtDepth, radiusAtDepth));
                        if (depth)
                        {
                            auto parentX = x >> 1U;
                            auto parentY = y >> 1U;
                            Node parent& = nodes[GetIndexAt(depth - 1), parentX, parentY];
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
            if (!root->rect.ContainsRect(object->rect))
            {
                return 0;
            }

            // shift object rect coords into range [0, 255]
            auto xMin = static_cast<uint8_t>(std::clamp(std::floor((rect.left   + radius) * inverseRadius), 0, 255));
            auto yMin = static_cast<uint8_t>(std::clamp(std::floor((rect.top    + radius) * inverseRadius), 0, 255));
            auto xMax = static_cast<uint8_t>(std::clamp(std::ceil( (rect.right  + radius) * inverseRadius), 0, 255));
            auto yMax = static_cast<uint8_t>(std::clamp(std::ceil( (rect.bottom + radius) * inverseRadius), 0, 255));

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

        void Insert(Object* objectPtr)
        {
            auto index = ChooseIndex(objectPtr->rect);
            nodes[index].InsertObject(objectPtr);
        }

    private:
        Node* root;
        double radius = 0;
        double inverseRadius = 0;
        std::array<Node, TotalNodes> nodes;

    };
}