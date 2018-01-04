#pragma once
#include <memory>
#define STB_IMAGE_IMPLEMENTATION
#include <stb-master/stb_image.h>

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
		};
	};
	using BitmapPtr = std::unique_ptr<stbi_uc, Deleter>;
	BitmapPtr m_Data;
	uint32_t m_Width;
	uint32_t m_Height;
	size_t m_Size;

	Bitmap(stbi_uc* bitmapPointer, uint32_t width, uint32_t height, size_t size) : 
		m_Data(bitmapPointer), m_Width(width), m_Height(height), m_Size(size) {}

	Bitmap& operator=(Bitmap&& other)
	{
		m_Data = std::move(other.m_Data);
		m_Width = std::move(other.m_Width);
		m_Height = std::move(other.m_Height);
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