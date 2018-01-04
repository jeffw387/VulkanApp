#pragma once
#include <string>
#include "Texture2D.hpp"

struct ImageMetadata
{
	std::string path;
	Texture2D texture;
};

template <typename... ImageTypes>
struct ImageManager
{
private:
	std::map<size_t, ImageMetadata> imageMap;

	template <typename ImageType>
	auto& getMetadata()
	{
		return imageMap[getID<ImageType>()];
	}
public:
	template <typename ImageType>
	auto getID()
	{
		return MPL::IndexOf<ImageType, ImageTypes...>::value;
	}

	template <typename ImageType>
	void initImage(VulkanApp& app, const char* path = ImageType::path)
	{
		auto& meta = getMetadata<ImageType>();
		meta.path = std::string(path);
		auto image = loadImage(meta.path);
		meta.texture = app.createTexture(std::move(image));
	}

	template <typename ImageType>
	const auto& getTexture()
	{
		return getMetadata<ImageType>().texture;
	}

	template <typename ImageType>
	const auto& getPath()
	{
		return getMetadata<ImageType>().path;
	}
};