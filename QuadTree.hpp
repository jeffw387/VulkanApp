#pragma once
#include <memory>
#include <array>
#include <bitset>

template <typename T>
struct QuadTreeRect2D
{
    T left, right, top, bottom;

    QuadTreeRect2D() noexcept = default;

    QuadTreeRect2D(T left, T right, T top, T bottom) :
        left(left), right(right), top(top), bottom(bottom)
    {}

    bool PointIsContained(const T x, const T y) const
    {
        return (x >= left) && (x <= right) && (y <= bottom) && (y >= top);
    }

    bool RectIsContained(const QuadTreeRect2D<T>& rect) const
    {
        return  (rect.left   >= this->left)  && 
                (rect.right  <= this->right) && 
                (rect.top    >= this->top)   && 
                (rect.bottom <= this->bottom);
    }

    T GetWidth() const
    {
        return right - left;
    }

    T GetHeight() const
    {
        return bottom - top;
    }
};



template <typename T, size_t MaxDepth = 6U, size_t MaxElementsPerLeaf = 32U>
class QuadTree
{
public:
    struct Quad
    {
        std::array<T, MaxElementsPerLeaf> elements;
        std::bitset<MaxElementsPerLeaf> isElementValid;
        bool isLeaf = true;
        QuadTreeRect2D<int64_t> rect;
        size_t depth = 0;
        std::array<std::unique_ptr<Quad>, 4> children;

        std::unique_ptr<Quad> TopLeftChild()        { return children[0] };
        std::unique_ptr<Quad> BottomLeftChild()     { return children[1] };
        std::unique_ptr<Quad> BottomRightChild()    { return children[2] };
        std::unique_ptr<Quad> TopRightChild()       { return children[3] };
    };

    struct QuadTreeHandle
    {
        Quad* quadPtr = nullptr;
        size_t elementIndex = 0;
        bool Valid()
        {
            if (quadPtr == nullptr)
                return false;
            return quadPtr->isElementValid(elementIndex);
        }
    };

    QuadTreeHandle InsertPoint(T newElement, const int64_t x, const int64_t y)
    {
        return InsertPointInQuad(root, newElement, x, y);
    }

private:
    Quad root;

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

    QuadTreeHandle InsertPointInQuad(Quad& quad, T newElement, const int64_t x, const int64_t y)
    {
        QuadTreeHandle result;
        if (quad.rect.PointIsContained(x, y))
        {
            if (quad.elements == nullptr)
            {
                quad.elements = std::make_unique<std::vector<T>>();
                quad.elements->resize(MaxElementsPerLeaf);
            }
            if (quad.elements->size() < MaxElementsPerLeaf)
            {
                quad.elements->push_back(T);
                result = &(quad.elements->back());
                return true;
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
            return false;
        }
    }
};