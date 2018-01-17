#pragma once
#include <memory>
#include "stb_image.h"

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