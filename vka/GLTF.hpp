#pragma once
#include "nlohmann/json.hpp"
#include "fileIO.hpp"
#include "glm/glm.hpp"
#include "gsl.hpp"
#include "UniqueVulkan.hpp"

#include <fstream>
#include <vector>
#include <string>
#include <cstring>

using json = nlohmann::json;

namespace vka
{
struct Mesh
{
	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<size_t> indices;
	vka::VkBufferUnique positionsBuffer;
	vka::VkBufferUnique normalsBuffer;
	vka::VkBufferUnique indexBuffer;
};

struct Node
{
	Mesh mesh;
};

struct Model
{
	Node collision;
	Node full;
};

namespace detail
{
using BufferVector = std::vector<std::vector<char>>;

template <typename T>
static void CopyFromBufferView(std::vector<T> &outputVector,
							   const json &accessor,
							   const json &j,
							   const BufferVector &buffers)
{
	size_t bufferViewIndex = accessor["bufferView"];
	json bufferView = j["bufferViews"][bufferViewIndex];

	size_t bufferIndex = bufferView["buffer"];
	size_t byteOffset = bufferView["byteOffset"];
	size_t byteLength = bufferView["byteLength"];
	auto &bufferData = buffers[bufferIndex];

	outputVector.resize(accessor["count"]);
	auto charData = bufferData.data();
	charData += byteOffset;
	std::memcpy(outputVector.data(), charData, byteLength);
}

static void CopyMeshData(Mesh &mesh, const json &primitive, const json &j, const BufferVector &buffers)
{
	size_t positionAccessorIndex = primitive["attributes"]["POSITION"];
	CopyFromBufferView(mesh.positions,
					   j["accessors"][positionAccessorIndex],
					   j,
					   buffers);

	size_t normalAccessorIndex = primitive["attributes"]["NORMAL"];
	CopyFromBufferView(mesh.normals,
					   j["accessors"][normalAccessorIndex],
					   j,
					   buffers);

	size_t indexAccessorIndex = primitive["indices"];
	CopyFromBufferView(mesh.indices,
					   j["accessors"][indexAccessorIndex],
					   j,
					   buffers);
}

static void LoadNode(Node &node, const json &nodejson, const json &j, const BufferVector &buffers)
{
	size_t meshIndex = nodejson["mesh"];
	auto &mesh = j["meshes"][meshIndex];
	auto &primitive = mesh["primitives"][0];
	CopyMeshData(node.mesh, primitive, j, buffers);
}
} // namespace detail

static Model LoadModelFromFile(std::string fileName)
{
	auto f = std::ifstream(fileName);
	json j;
	f >> j;

	detail::BufferVector buffers;

	for (const auto &buffer : j["buffers"])
	{
		buffers.push_back(fileIO::readFile(buffer["uri"]));
	}

	Model model;

	for (const auto &node : j["nodes"])
	{
		if (node["name"] == "Collision")
		{
			detail::LoadNode(model.collision, node, j, buffers);
		}
		else
		{
			detail::LoadNode(model.full, node, j, buffers);
		}
	}
	return model;
}
} // namespace vka
