#pragma once
#include <memory>
#include <array>
#include <bitset>

namespace QuadTree
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
    };

    template <typename T, size_t MaxDepth = 6U, size_t MaxElementsPerLeaf = 16U>
    class Tree
    {
    public:
        struct Quad
        {
            std::array<T, MaxElementsPerLeaf> elements;
            std::array<Point, MaxElementsPerLeaf> elementPositions;
            std::bitset<MaxElementsPerLeaf> isElementValid;
            bool isLeaf = true;
            Rect rect;
            size_t depth = 0;
            std::array<std::unique_ptr<Quad>, 4> children;

            std::unique_ptr<Quad> TopLeftChild()        { return children[0] };
            std::unique_ptr<Quad> BottomLeftChild()     { return children[1] };
            std::unique_ptr<Quad> BottomRightChild()    { return children[2] };
            std::unique_ptr<Quad> TopRightChild()       { return children[3] };
        };

        struct Handle
        {
        private:
            Quad* quadPtr = nullptr;
            size_t elementIndex = 0;
        };

        Handle InsertPoint(const T& newElement, const Point& point)
        {
            return InsertPointInQuad(root, point, newElement);
        }

        Handle InsertPoint(const T& newElement, const int64_t x, const int64_t y)
        {
            return InsertPointInQuad(root, Point(x, y), newElement);
        }

        const Quad& GetElementsInRegion(const Rect& rect)
        {
            return GetElementsInRegion(root, rect);
        }

    private:
        Quad root;

        auto SetElementAtIndex(Quad& quad, const Point& point, const T& newElement, const size_t i)
        {
            quad.elementPositions[i] = point;
            quad.elements[i] = newElement;
            quad.isElementValid[i].set();

            Handle result;
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

            // move elements into children
            for (auto i = 0U; i < MaxElementsPerLeaf; i++)
            {
                for (auto& child : quad.children)
                {
                    if (InsertPointInQuad(child, quad.elementPositions[i], quad.elements[i]).has_value())
                    {
                        break;
                    }
                }
            }
            quad.isElementValid.reset();
            quad.isLeaf = false;

            return true;
        }

        std::optional<Handle> InsertPointInQuad(Quad& quad, const Point& point, const T& newElement)
        {
            std::optional<Handle> result;
            if (!quad.isLeaf)
                return result;
            if (quad.rect.ContainsPoint(x, y))
            {
                if (!quad.isElementValid.all())
                {
                    for (auto i = 0U; i < MaxElementsPerLeaf; i++)
                    {
                        if (!quad.isElementValid[i])
                        {
                            result = SetElementAtIndex(quad, point, newElement, i);
                            return result;
                        }
                    }
                }
                if (SubdivideQuad(quad))
                {
                    for (auto& child : quad.children)
                    {
                        result = InsertPointInQuad(child, point, newElement);
                        if (result.has_value())
                        {
                            return result;
                        }
                    }
                }
            }
            return result;
        }

        const Quad& GetElementsInRegion(const Quad& quad, const Rect& rect)
        {
            for (auto& child : quad.children)
            {
                if (child.rect.ContainsRect(rect))
                {
                    return GetElementsInRegion(child, rect);
                }
            }
            return quad;
        }
    };
}