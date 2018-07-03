#pragma once

#include "nlohmann/json.hpp"
#include <fstream>

namespace vka
{
	using json = nlohmann::json;

	static json GetModule(const json& inputJson)
	{
		auto jsonID = inputJson.find("jsonID");
		if (jsonID != inputJson.end())
		{
			bool isInline = jsonID["inline"];
			if (isInline)
			{
				return inputJson;
			}
			else
			{
				auto f = std::ifstream(jsonID["uri"]);
				json j;
				f >> j;
				return j;
			}
		}
	}
}