#pragma once
#include <memory>
#include <vector>
#include <stdexcept>

template <typename T>
class QuadTree
{
public:
    struct Quad
    {
        float left, right, top, bottom;
        std::unique_ptr<std::vector<T>> leafData;

        bool PointIsContained(const float x, const float y) const
        {
            return (x >= left) && (x <= right) && (y <= bottom) && (y >= top);
        }

        bool RectIsContained(const float left, const float right, const float top, const float bottom) const
        {
            return (left >= this->left) && (right <= this->right) && (top >= this->top) && (bottom <= this->bottom);
        }
    };

    void InsertPoint(T newElement, const float x, const float y)
    {
        InsertPointInQuad(root, newElement, x, y);
    }


private:
    Quad root;
    size_t m_MaxLeafMembers = 4;
    float m_MinimumSideLength;

    void InsertPointInQuad(Quad& quad, T newElement, const float x, const float y)
    {
        if (quad.PointIsContained(x, y))
        {
            if (quad.leafData == nullptr)
            {
                quad.leafData = std::make_unique<std::vector<T>>();
                quad.leafData->reserve(m_MaxLeafMembers);
            }
            std::runtime_error("Point is not contained within borders of quadtree.");
        }
    }
};