#pragma once

#include "Vertex.hpp"

namespace vka {
struct Quad {
  static constexpr size_t VertexCount = 6U;
  static constexpr size_t QuadSize = sizeof(Vertex2D) * VertexCount;
  std::array<Vertex2D, VertexCount> vertices;
  size_t firstVertex;
};
}  // namespace vka