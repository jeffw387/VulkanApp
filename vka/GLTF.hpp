#pragma once
#include "nlohmann/json.hpp"
#include "fileIO.hpp"
#include "glm/glm.hpp"
#include "gsl.hpp"
#include "UniqueVulkan.hpp"
#include "entt/entt.hpp"

#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>

using json = nlohmann::json;

namespace vka
{
	struct Mesh
	{
		std::vector<glm::vec3> positions;
		std::vector<glm::vec3> normals;
		std::vector<uint8_t> indices;
		size_t firstIndex;
		size_t firstVertex;
	};

	struct glTF
	{
		Mesh collision;
		Mesh full;
	};

	namespace detail
	{
		using BufferVector = std::vector<std::vector<char>>;

		template <typename T>
		static void CopyFromBufferView(
			std::vector<T> &outputVector,
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

		static void LoadMesh(
			Mesh &mesh, 
			const json &nodejson, 
			const json &j, 
			const BufferVector &buffers)
		{
			size_t meshIndex = nodejson["mesh"];
			auto &primitive = j["meshes"][meshIndex]["primitives"][0];

			size_t indexAccessorIndex = primitive["indices"];
			auto indexAccessor = j["accessors"][indexAccessorIndex];
			CopyFromBufferView(mesh.indices, indexAccessor, j, buffers);

			size_t positionAccessorIndex = primitive["attributes"]["POSITION"];
			auto positionAccessor = j["accessors"][positionAccessorIndex];
			CopyFromBufferView(mesh.positions, positionAccessor, j, buffers);

			size_t normalAccessorIndex = primitive["attributes"]["NORMAL"];
			auto normalAccessor = j["accessors"][normalAccessorIndex];
			CopyFromBufferView(mesh.normals, normalAccessor, j, buffers);
		}
	} // namespace detail

	static glTF LoadModel(entt::HashedString fileName)
	{
		auto f = std::ifstream(std::string(fileName));
		json j;
		f >> j;

		detail::BufferVector buffers;

		for (const auto &buffer : j["buffers"])
		{
			std::string bufferFileName = buffer["uri"];
			buffers.push_back(fileIO::readFile(bufferFileName));
		}

		auto model = glTF{};

		for (const auto &node : j["nodes"])
		{
			if (node["name"] == "Collision")
			{
				detail::LoadMesh(model.collision, node, j, buffers);
			}
			else
			{
				detail::LoadMesh(model.full, node, j, buffers);
			}
		}
		return model;
	}

} // namespace vka
