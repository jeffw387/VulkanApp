#pragma once
#include <array>
#include <vector>
#include "IntrusiveList.hpp"

namespace QT
{
    struct Rect
    {
        Rect() noexcept = default;

        Rect(int64_t left, int64_t right, int64_t top, int64_t bottom) :
            left(left), right(right), top(top), bottom(bottom), width(right - left), height(bottom - top)
        {}

        bool ContainsPoint(const Point& point)
        {
            return ContainsPoint(point.x, point.y);
        }

        bool ContainsPoint(const int64_t x, const int64_t y) const
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

        int64_t GetWidth()
        {
            return width;
        }

        int64_t GetHeight()
        {
            return height;
        }

        int64_t left, right, top, bottom;

    private:
        int64_t width, height;
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
        private:
            IntrusiveList<Object> objects;
            std::array<Node*>, 4> children;
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

        Tree() noexcept = default;
        Tree(const Rect& region) : root(region)
        {}

    private:
        Node* root;
        std::array<Node, TotalNodes> nodes;

    };
}