#pragma once
#include <memory>
#include <array>
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
        static constexpr size_t TotalElements = 1 + (2*2) + (4*4) + (8*8) + (16*16) + (32*32) + (64*64) + (128*128);

        struct Object
        {
            Rect boundingBox;
            Quad* parentQuad;
        };

        struct Quad
        {
        public:
            Quad() noexcept = default;
            Quad(const Rect& region) : rect(region)
            {}

        private:
            std::forward_list<Object> objects;
            Rect rect;
            std::array<std::unique_ptr<Quad>, 4> children;
        };

        struct Handle
        {
            friend class Tree;
        private:
            Quad* quadPtr = nullptr;
            size_t elementIndex = 0;
        };

        Tree() noexcept = default;
        Tree(const Rect& region) : root(region)
        {}

        const Quad& GetElementsInRegion(const Rect& rect)
        {
            return GetElementsInRegion(root, rect);
        }

        Rect GetRegion()
        {
            return root.rect;
        }

    private:
        std::unique_ptr<Quad> root;
        std::array<T, TotalElements> elements;

    };
}