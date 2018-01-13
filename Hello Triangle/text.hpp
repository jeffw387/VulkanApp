#pragma once
#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include <iostream>
#include <utility>
#include "Bitmap.hpp"
#include <memory>
#include <gsl/gsl>
#include "Texture2D.hpp"

namespace Text
{
using FontID = size_t;
using FontSize = size_t;
using GlyphID = FT_UInt;
using CharCode = FT_ULong;

struct GlyphData
{
	FT_Glyph glyph;
	TextureIndex textureIndex;
};

using GlyphMap = std::map<CharCode, GlyphData>;

struct SizeData
{
	GlyphMap glyphMap;
	FT_F26Dot6 spaceAdvance;
};

struct FontData
{
    FT_Face face;
    std::string path;
    std::map<FontSize, SizeData> glyphsBySize;
};

class TextEngine
{
public:
    FT_Library m_Library;
    std::map<FontID, FontData> m_FontMaps;

    void init()
    {
        auto error = FT_Init_FreeType(&m_Library);
        if (error != 0)
        {
            std::cerr << "Error initializing Freetype!";
        }
    }

	void cleanup()
	{
		for (auto& [fontID, fontData] : m_FontMaps)
		{
			for (auto& [size, sizeData] : fontData.glyphsBySize)
			{
				for (auto& [charcode, glyphData] : sizeData.glyphMap)
				{
					FT_Done_Glyph(glyphData.glyph);
				}
			}
		}
		auto error = FT_Done_FreeType(m_Library);
	}

	SizeData& getSizeData(FontID fontID, FontSize size)
	{
		auto& fontData = m_FontMaps.at(fontID);
		return fontData.glyphsBySize.at(size);
	}

	Bitmap getFullBitmap(FT_Glyph glyph)
	{
		auto bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);
		auto& sourceBitmap = bitmapGlyph->bitmap;
		constexpr auto channelCount = 4U;
		auto outputSize = sourceBitmap.width * sourceBitmap.rows * channelCount;

		// Have to flip the bitmap so bottom row in source is top in output
		std::vector<unsigned char> pixels;
		pixels.reserve(outputSize);
		for (int sourceRow = sourceBitmap.rows - 1; sourceRow >= 0; sourceRow--)
		{
			for (size_t sourceColumn = 0; sourceColumn < sourceBitmap.width; sourceColumn++)
			{
				size_t sourcePosition = (sourceRow * sourceBitmap.pitch) + sourceColumn;

				auto source = sourceBitmap.buffer;
				pixels.push_back(255);
				pixels.push_back(255);
				pixels.push_back(255);
				pixels.push_back(source[sourcePosition]);
			}
		}
		FT_BBox box;
		FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_PIXELS, &box);

		Bitmap resultBitmap = Bitmap(
			reinterpret_cast<stbi_uc*>(pixels.data()), 
			sourceBitmap.width, 
			sourceBitmap.rows, 
			box.xMin,
			box.yMax,
			outputSize
		);
		return resultBitmap;
	}

	std::optional<FT_Glyph> getGlyph(FontID fontID, FontSize size, CharCode charcode)
	{
		SizeData& sizeData = getSizeData(fontID, size);
		auto it = sizeData.glyphMap.find(charcode);
		std::optional<FT_Glyph> glyphOptional;
		if (it != sizeData.glyphMap.end())
		{
			glyphOptional = it.operator*().second.glyph;
		}
		return glyphOptional;
	}

	auto getAdvance(FontID fontID, FontSize size, CharCode left)
	{
		auto glyphOptional = getGlyph(fontID, size, left);
		if (glyphOptional)
		{
			// 16.16 fixed point to 32 int
			return static_cast<FT_Int32>((*glyphOptional)->advance.x >> 16);
		}
		else
		{
			// 16.16 fixed point to 32 int
			auto advance = getSizeData(fontID, size).spaceAdvance;
			auto advanceShifted = advance >> 6;
			return static_cast<FT_Int32>(advanceShifted);
		}
	}

	auto getAdvance(FontID fontID, FontSize size, CharCode left, CharCode right)
	{
		auto advance = getAdvance(fontID, size, left);
		auto face = m_FontMaps[fontID].face;
		GlyphID leftGlyph = FT_Get_Char_Index(face, left);
		GlyphID rightGlyph = FT_Get_Char_Index(face, right);
		FT_Vector kerning = { 0, 0 };
		if (FT_HAS_KERNING(face))
		{
			FT_Get_Kerning(face, leftGlyph, rightGlyph, FT_KERNING_DEFAULT, &kerning);
		}
		advance += kerning.x >> 6;
		return advance;
	}

}; // class TextEngine
} // namespace Text
