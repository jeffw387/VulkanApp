#pragma once
#include <memory>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

struct Bitmap
{
	struct Deleter
	{
		Deleter() {}
		Deleter(const Deleter&) {}
		Deleter(Deleter&&) {}

		Deleter& operator=(const Deleter& other)
		{
			return *this;
		}
		
		void operator()(stbi_uc* bitmap) const
		{
			stbi_image_free(bitmap);
		}
	};
	using BitmapPtr = std::unique_ptr<stbi_uc, Deleter>;
	BitmapPtr m_Data;
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_Origin_x, m_Origin_y;
	size_t m_Size;

	Bitmap(stbi_uc* bitmapPointer, uint32_t width, uint32_t height, 
		uint32_t origin_x, uint32_t origin_y, size_t size) : 
		m_Data(bitmapPointer), m_Width(width), m_Height(height), 
		m_Origin_x(origin_x), m_Origin_y(origin_y), m_Size(size) {}

	Bitmap& operator=(Bitmap&& other)
	{
		m_Data = std::move(other.m_Data);
		m_Width = std::move(other.m_Width);
		m_Height = std::move(other.m_Height);
		m_Origin_x = std::move(other.m_Origin_x);
		m_Origin_y = std::move(other.m_Origin_y);
		m_Size = std::move(other.m_Size);
		return *this;
	}

	Bitmap& operator=(const Bitmap&) = delete;
	Bitmap(const Bitmap&) = delete;

	Bitmap(Bitmap&& other)
	{
		(*this).operator=(std::move(other));
	}
};