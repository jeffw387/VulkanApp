#pragma once
#include <fstream>
#include <vector>

namespace fileIO
{
	static std::vector<char> readFile(const std::string& filename) 
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) 
		{
			throw std::runtime_error("failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer;
		buffer.resize(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		return buffer;
	}
}