#pragma once
#include <memory>
#include <stb_image.h>

struct Bitmap
{
	std::vector<unsigned char> m_Data;
	uint32_t m_Width, m_Height;
	int32_t m_Left, m_Top;
	size_t m_Size;

	Bitmap(stbi_uc* pixelData, uint32_t width, uint32_t height, 
		int32_t left, int32_t top, size_t size) : 
		m_Data(pixelData, pixelData + size), m_Width(width), m_Height(height), 
		m_Left(left), m_Top(top), m_Size(size) {}
};

inline Bitmap loadImageFromFile(std::string path)
{
	// Load the texture into an array of pixel data
	int channels_i, width_i, height_i;
	auto pixels = stbi_load(path.c_str(), &width_i, &height_i, &channels_i, STBI_rgb_alpha);
	uint32_t width, height;
	width = static_cast<uint32_t>(width_i);
	height = static_cast<uint32_t>(height_i);
	Bitmap image = Bitmap(pixels, width, height, static_cast<int32_t>(std::ceil(width * -0.5f)), static_cast<int32_t>(std::ceil(height * 0.5f)), width * height * 4U);

	return image;
}