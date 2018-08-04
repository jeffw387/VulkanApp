#pragma once

namespace vka {
struct Quad {
  static constexpr size_t ElementCount = 12U;
  static constexpr size_t QuadSize = sizeof(glm::vec2) * ElementCount;
  std::array<glm::vec2, 12> vertices;
  size_t firstVertex;
};
}  // namespace vka