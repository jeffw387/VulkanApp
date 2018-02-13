#pragma once
#include <memory>
#include <array>
#include <bitset>
#include <optional>
#include <forward_list>

namespace QuadTree
{
    struct Point
    {
        int64_t x, y;
        Point() noexcept = default;
        Point(int64_t x, int64_t y) : x(x), y(y)
        {}
    };

    bool operator==(const Point& a, const Point& b)
    {
        return (a.x == b.x) && (a.y == b.y);
    }

    struct Rect
    {
        int64_t left, right, top, bottom;
    private: 
        int64_t width, height;
    public:
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

        std::optional<Handle> InsertPoint(const T& newElement, const Point& point)
        {
            return InsertPointInQuad(root, point, newElement);
        }

        std::optional<Handle> InsertPoint(const T& newElement, const int64_t x, const int64_t y)
        {
            return InsertPointInQuad(root, Point(x, y), newElement);
        }

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

        auto SetElementAtIndex(Quad& quad, const Point& point, const T& newElement, const size_t i)
        {
            quad.elementPositions[i] = point;
            quad.elements[i] = newElement;
            quad.isElementValid.set(i);

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
            auto newWidth = static_cast<double>(quad.rect.GetWidth()) / 2;
            auto newHeight = static_cast<double>(quad.rect.GetHeight()) / 2;

            auto CenterLeftEdge     = static_cast<int64_t>(std::floor(newWidth));
            auto CenterRightEdge    = static_cast<int64_t>(std::ceil(newWidth));
            auto CenterTopEdge      = static_cast<int64_t>(std::floor(newHeight));
            auto CenterBottomEdge   = static_cast<int64_t>(std::ceil(newHeight));

            quad.children[0] = std::make_unique<Quad>();
            quad.children[0]->depth = newDepth;
            quad.children[0]->rect = 
                Rect(quad.rect.left, 
                    quad.rect.left + CenterLeftEdge,
                    quad.rect.top,
                    quad.rect.top + CenterTopEdge);
            
            quad.children[1] = std::make_unique<Quad>();
            quad.children[1]->depth = newDepth;
            quad.children[1]->rect = 
                Rect(quad.rect.left,
                    quad.rect.left + CenterLeftEdge,
                    quad.rect.top + CenterBottomEdge,
                    quad.rect.bottom);

            quad.children[2] = std::make_unique<Quad>();
            quad.children[2]->depth = newDepth;
            quad.children[2]->rect = 
                Rect(quad.rect.left + CenterRightEdge,
                quad.rect.right,
                quad.rect.top + CenterBottomEdge,
                quad.rect.bottom);

            quad.children[3] = std::make_unique<Quad>();
            quad.children[3]->depth = newDepth;
            quad.children[3]->rect =
                Rect(quad.rect.left + CenterRightEdge,
                quad.rect.right,
                quad.rect.top,
                quad.rect.top + CenterTopEdge);

            // move elements into children
            for (auto i = 0U; i < MaxElementsPerLeaf; i++)
            {
                for (auto& child : quad.children)
                {
                    auto result = InsertPointInQuad(*child, quad.elementPositions[i], quad.elements[i]);
                    if (result.has_value())
                    {
                        break;
                    }
                }
            }
            quad.isElementValid.reset();

            return true;
        }

        std::optional<Handle> InsertPointInQuad(Quad& quad, const Point& point, const T& newElement)
        {
            std::optional<Handle> result;
            if (quad.rect.ContainsPoint(point))
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
                else
                {
                    SubdivideQuad(quad);
                }
                for (auto& child : quad.children)
                {
                    result = InsertPointInQuad(*child, point, newElement);
                    if (result.has_value())
                    {
                        return result;
                    }
                }
            }
            return result;
        }

        const Quad& GetElementsInRegion(const Quad& quad, const Rect& rect)
        {
            for (auto& child : quad.children)
            {
                if (child->rect.ContainsRect(rect))
                {
                    return GetElementsInRegion(*child, rect);
                }
            }
            return quad;
        }
    };
}