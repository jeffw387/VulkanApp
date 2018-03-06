#pragma once

#include "entt.hpp"
#include "Sprite.hpp"
#include "VulkanApp2.hpp"

#ifndef CONTENTROOT
#define CONTENTROOT
#endif

struct Image2DLoader final: entt::ResourceLoader<Image2DLoader, Image2D>
{
    std::shared_ptr<Image2D> load(VulkanApp& app, const std::string& imagePath, const std::string& jsonPath)
    {
        
    }
};
namespace Image
{
	std::array<std::string, static_cast<size_t>(Image::COUNT)> Paths =
	{
		CONTENTROOT "Content/Textures/star.png",
		CONTENTROOT "Content/Textures/texture.jpg",
		CONTENTROOT "Content/Textures/dungeon_sheet.png"
	};
}