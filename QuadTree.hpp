#pragma once
#include <memory>
#include <array>
#include <bitset>

namespace quadtree
{
    struct Point
    {
        int64_t x, y;
        Point noexcept = default;
        Point(int64_t x, int64_t y) : x(x), y(y)
        {}
    };

    struct Rect
    {
        int64_t left, right, top, bottom, width, height;
        
        Rect() noexcept = default;

        Rect(int64_t left, int64_t right, int64_t top, int64_t bottom) :
            left(left), right(right), top(top), bottom(bottom), width(right - left), height(bottom - top)
        {}

        bool PointIsContained(const Point& point)
        {
            return PointIsContained(point.x, point.y);
        }

        bool PointIsContained(const int64_t x, const int64_t y) const
        {
            return (x >= left) && (x <= right) && (y <= bottom) && (y >= top);
        }

        bool RectIsContained(const Rect& rect) const
        {
            return  (rect.left   >= this->left)  && 
                    (rect.right  <= this->right) && 
                    (rect.top    >= this->top)   && 
                    (rect.bottom <= this->bottom);
        }
    };

    template <typename T, size_t MaxDepth = 6U, size_t MaxElementsPerLeaf = 16U>
    class QuadTree
    {
    public:
        struct QuadElement
        {
            int64_t x, y;
            T element;
        };

        struct Quad
        {
            std::array<QuadElement, MaxElementsPerLeaf> elements;
            std::bitset<MaxElementsPerLeaf> isElementValid;
            bool isLeaf = true;
            Rect<int64_t> rect;
            size_t depth = 0;
            std::array<std::unique_ptr<Quad>, 4> children;

            std::unique_ptr<Quad> TopLeftChild()        { return children[0] };
            std::unique_ptr<Quad> BottomLeftChild()     { return children[1] };
            std::unique_ptr<Quad> BottomRightChild()    { return children[2] };
            std::unique_ptr<Quad> TopRightChild()       { return children[3] };
        };

        struct QuadTreeHandle
        {
        private:
            Quad* quadPtr = nullptr;
            size_t elementIndex = 0;
        };

        QuadTreeHandle InsertPoint(T newElement, const int64_t x, const int64_t y)
        {
            return InsertPointInQuad(root, newElement, Rect<int64_t>(x, x, y, y);
        }

    private:
        Quad root;

        auto SetElementAtIndex(Quad& quad, const Rect<int64_t>& rect, const T& newElement, const size_t i)
        {
            QuadTreeHandle result;
            quad.elements[i].rect = rect;
            quad.elements[i].element = newElement;
            result.quadPtr = &quad;
            result.elementIndex = i;
            return result;
        }

        bool SubdivideQuad(Quad& quad)
        {
            if (quad.depth == MaxDepth)
                return false;
            auto newDepth = quad.depth + 1;
            auto newWidth = static_cast<double>(quad.GetWidth()) / 2;
            auto newHeight = static_cast<double>(quad.GetHeight()) / 2;

            auto CenterLeftEdge     = static_cast<int64_t>(std::floor(newWidth));
            auto CenterRightEdge    = static_cast<int64_t>(std::ceil(newWidth));
            auto CenterTopEdge      = static_cast<int64_t>(std::floor(newHeight));
            auto CenterBottomEdge   = static_cast<int64_t>(std::ceil(newHeight));

            quad.isLeaf = false;

            quad.children[0] = make_unique<Quad>();
            quad.children[0]->left = quad.rect.left;
            quad.children[0]->right = quad.rect.left + CenterLeftEdge;
            quad.children[0]->top = quad.rect.top;
            quad.children[0]->bottom = quad.rect.top + CenterTopEdge;
            quad.children[0]->depth = newDepth;

            quad.children[1] = make_unique<Quad>();
            quad.children[1]->left = quad.rect.left;
            quad.children[1]->right = quad.rect.left + CenterLeftEdge;
            quad.children[1]->top = quad.rect.top + CenterBottomEdge;
            quad.children[1]->bottom = quad.rect.bottom;
            quad.children[1]->depth = newDepth;

            quad.children[2] = make_unique<Quad>();
            quad.children[2]->left = quad.rect.left + CenterRightEdge;
            quad.children[2]->right = quad.rect.right;
            quad.children[2]->top = quad.rect.top + CenterBottomEdge;
            quad.children[2]->bottom = quad.rect.bottom;
            quad.children[2]->depth = newDepth;

            quad.children[3] = make_unique<Quad>();
            quad.children[3]->left = quad.rect.left + CenterRightEdge;
            quad.children[3]->right = quad.rect.right;
            quad.children[3]->top = quad.rect.top;
            quad.children[3]->bottom = quad.rect.top + CenterTopEdge;
            quad.children[3]->depth = newDepth;

            return true;
        }

        QuadTreeHandle InsertPointInQuad(Quad& quad, const Rect<int64_t>& rect, const T& newElement)
        {
            QuadTreeHandle result;
            if (quad.rect.PointIsContained(x, y))
            {
                if (!quad.isElementValid.all())
                {
                    for (auto i = 0U; i < MaxElementsPerLeaf; i++)
                    {
                        if (!quad.isElementValid[i])
                        {
                            return SetElementAtIndex(quad, rect, newElement, i);
                        }
                    }
                }
                if (SubdivideQuad(quad))
                {
                    for (auto& child : quad.children)
                    {
                        if (InsertPointInQuad(child, newElement, x, y))
                        {
                            return true;
                        }
                    }
                }
            }
            return result;
        }
    };
}