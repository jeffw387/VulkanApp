#pragma once
#include "UniqueVulkan.hpp"
#include "entt/entt.hpp"
#include "fileIO.hpp"
#include "glm/glm.hpp"
#include "gsl.hpp"
#include "nlohmann/json.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <variant>
#include <vector>

using json = nlohmann::json;

namespace vka {
using IndexType = uint16_t;
static constexpr VkIndexType VulkanIndexType = VK_INDEX_TYPE_UINT16;
struct Mesh {
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<IndexType> indices;
  size_t firstIndex;
  size_t firstVertex;
};

struct glTF {
  Mesh collision;
  Mesh full;
};

namespace detail {
using BufferVector = std::vector<std::vector<char>>;

std::map<size_t, size_t> sizeMap = {{5120, 1}, {5121, 1}, {5122, 2},
                                    {5123, 2}, {5125, 4}, {5126, 4}};

std::map<std::string, size_t> componentCountMap = {
    {"SCALAR", 1}, {"VEC2", 2}, {"VEC3", 3}, {"VEC4", 4},
    {"MAT2", 4},   {"MAT3", 9}, {"MAT4", 16}};

template <typename ComponentType, typename T>
static void CopyFromBufferView(std::vector<T> &outputVector,
                               const json &accessor, const json &j,
                               const BufferVector &buffers) {
  size_t bufferViewIndex = accessor["bufferView"];
  size_t componentType = accessor["componentType"];
  std::string elementType = accessor["type"];
  size_t elementCount = accessor["count"];
  size_t inputComponentSize = sizeMap[componentType];
  size_t elementComponentCount = componentCountMap[elementType];
  size_t elementSize = inputComponentSize * elementComponentCount;
  json bufferView = j["bufferViews"][bufferViewIndex];

  size_t bufferIndex = bufferView["buffer"];
  size_t byteOffset = bufferView["byteOffset"];
  size_t byteLength = bufferView["byteLength"];
  auto &bufferData = buffers[bufferIndex];

  outputVector.resize(elementCount);

  auto inputData = (unsigned char *)bufferData.data();
  inputData += byteOffset;
  auto outputData = (ComponentType *)outputVector.data();
  if (componentType == 5120) {
    int8_t *int8Ptr = (int8_t *)inputData;
    for (auto currentComponent = 0U;
         currentComponent < elementCount * elementComponentCount;
         ++currentComponent) {
      *(outputData + currentComponent) = *(int8Ptr + currentComponent);
    }
  }
  if (componentType == 5121) {
    uint8_t *uint8Ptr = (uint8_t *)inputData;
    for (auto currentComponent = 0U;
         currentComponent < elementCount * elementComponentCount;
         ++currentComponent) {
      *(outputData + currentComponent) = *(uint8Ptr + currentComponent);
    }
  }
  if (componentType == 5122) {
    int16_t *int16Ptr = (int16_t *)inputData;
    for (auto currentComponent = 0U;
         currentComponent < elementCount * elementComponentCount;
         ++currentComponent) {
      *(outputData + currentComponent) = *(int16Ptr + currentComponent);
    }
  }
  if (componentType == 5123) {
    uint16_t *uint16Ptr = (uint16_t *)inputData;
    for (auto currentComponent = 0U;
         currentComponent < elementCount * elementComponentCount;
         ++currentComponent) {
      *(outputData + currentComponent) = *(uint16Ptr + currentComponent);
    }
  }
  if (componentType == 5125) {
    uint32_t *uint32Ptr = (uint32_t *)inputData;
    for (auto currentComponent = 0U;
         currentComponent < elementCount * elementComponentCount;
         ++currentComponent) {
      *(outputData + currentComponent) = *(uint32Ptr + currentComponent);
    }
  }
  if (componentType == 5126) {
    float *float32Ptr = (glm::float32 *)inputData;
    for (auto currentComponent = 0U;
         currentComponent < elementCount * elementComponentCount;
         ++currentComponent) {
      *(outputData + currentComponent) = *(float32Ptr + currentComponent);
    }
  }
}

static void LoadMesh(Mesh &mesh, const json &nodejson, const json &j,
                     const BufferVector &buffers) {
  size_t meshIndex = nodejson["mesh"];
  auto &primitive = j["meshes"][meshIndex]["primitives"][0];

  size_t indexAccessorIndex = primitive["indices"];
  auto indexAccessor = j["accessors"][indexAccessorIndex];
  CopyFromBufferView<IndexType>(mesh.indices, indexAccessor, j, buffers);

  size_t positionAccessorIndex = primitive["attributes"]["POSITION"];
  auto positionAccessor = j["accessors"][positionAccessorIndex];
  CopyFromBufferView<glm::float32>(mesh.positions, positionAccessor, j,
                                   buffers);

  size_t normalAccessorIndex = primitive["attributes"]["NORMAL"];
  auto normalAccessor = j["accessors"][normalAccessorIndex];
  CopyFromBufferView<glm::float32>(mesh.normals, normalAccessor, j, buffers);
}
}  // namespace detail

static glTF LoadModel(const char *fileName) {
  std::filesystem::path modelPath = std::string(fileName);
  auto f = std::ifstream(modelPath);
  json j;
  f >> j;

  detail::BufferVector buffers;

  for (const auto &buffer : j["buffers"]) {
    std::filesystem::path bufferPath = modelPath;
    std::string bufferFileName = buffer["uri"];
    bufferPath.replace_filename(bufferFileName);
    buffers.push_back(fileIO::readFile(bufferPath.string()));
  }

  auto model = glTF{};

  for (const auto &node : j["nodes"]) {
    if (node["name"] == "Collision") {
      detail::LoadMesh(model.collision, node, j, buffers);
    } else {
      detail::LoadMesh(model.full, node, j, buffers);
    }
  }
  return model;
}

}  // namespace vka
