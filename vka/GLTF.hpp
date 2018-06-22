#pragma once
#include "nlohmann/json.hpp"
#include "fileIO.hpp"
#include "glm/glm.hpp"
#include "gsl.hpp"

#include <fstream>
#include <vector>

using json = nlohmann::json;

namespace vka
{
	struct glTF
	{
		json glTFjson;
		std::vector<std::vector<char>> buffers;
	};

	static glTF LoadModelFromFile(std::string fileName)
	{
		auto f = std::ifstream(fileName);
		glTF glTFData;
		f >> glTFData.glTFjson;

		for (const auto& buffer : glTFData.glTFjson["buffers"])
		{
			glTFData.buffers.push_back(fileIO::readFile(buffer["uri"]));
		}

		return std::move(glTFData);
	}

	template <typename T>
	static gsl::span<T> GetSpan(const glTF& glTFData, const json& bufferView)
	{
		const auto& buffer = glTFData.buffers[bufferView["buffer"]];
		auto bufferData = buffer.data();
		auto byteOffset = static_cast<size_t>(bufferView["byteOffset"]);
		auto start = bufferData + byteOffset;
		auto byteLength = static_cast<size_t>(bufferView["byteLength"]);
		auto result = gsl::span<T>(reinterpret_cast<T*>(start), byteLength);
	}

	static const json& GetBufferView(const json& glTFjson, const json& accessor)
	{
		size_t bufferViewIndex = accessor["bufferView"];
		return glTFjson["bufferViews"][bufferViewIndex];
	}

	static const json& GetAccessor(const json& glTF)
	{
		
	}

	static const json& GetScene(const json& glTFjson)
	{
		size_t primarySceneIndex = glTFjson["scene"];
		return glTFjson[primarySceneIndex];
	}

	

}