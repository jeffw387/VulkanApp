#pragma once

#include "nlohmann/json.hpp"
#include <fstream>
#include <map>
#include <string>

namespace vka
{
	using json = nlohmann::json;

	static json LoadJson(const std::string path)
	{
		std::ifstream f(path);
		json j;
		f >> j;
		return j;
	}

	static json GetModule(const json& inputJson)
	{
		auto jsonID = inputJson.find("jsonID");
		if (jsonID != inputJson.end())
		{
			bool isInline = (*jsonID)["inline"];
			if (isInline)
			{
				return inputJson;
			}
			else
			{
				auto f = std::ifstream((*jsonID)["uri"]);
				json j;
				f >> j;
				return j;
			}
		}
	}
	
	template <typename moduleT>
	static moduleT&& StoreHandle(
		moduleT&& module, 
		const json& moduleJson, 
		std::map<std::string, moduleT>& moduleMap)
	{
		std::string uri = moduleJson["jsonID"]["uri"];
		moduleMap[uri] = module;

		return std::move(module);
	}
}