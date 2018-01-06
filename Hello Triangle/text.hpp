#pragma once
#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include <iostream>
#include "MPL.hpp"
#include <utility>
#include "Bitmap.hpp"
#include <memory>
#include <gsl/gsl>

namespace Text
{
using FontID = size_t;
using FontSize = size_t;
using GlyphID = FT_UInt;
using CharCode = FT_ULong;

struct GlyphDeleter
{
	GlyphDeleter() {}
	GlyphDeleter(const GlyphDeleter&) {}
	GlyphDeleter(GlyphDeleter&&) {}

	GlyphDeleter& operator=(const GlyphDeleter& other)
	{
		return *this;
	}

	void operator()(FT_Glyph* glyph) const
	{
		FT_Done_Glyph(*glyph);
	}
};
using GlyphPtr = std::unique_ptr<FT_Glyph, GlyphDeleter>;

using GlyphMap = std::map<CharCode, GlyphPtr>;

struct FontData
{
    FT_Face face;
    std::string path;
    std::map<FontSize, GlyphMap> glyphsBySize;
};

template <typename... fontTypes>
class TextEngine
{
    std::map<FontID, FontData> fontMaps;
    FT_Library library;

    template <typename FontType>
    auto getFontID()
    {
        return static_cast<FontID>(MPL::IndexOf<FontType, fontTypes...>::value);
    }

public:
    void init()
    {
        auto error = FT_Init_FreeType(&library);
        if (error != 0)
        {
            std::cerr << "Error initializing Freetype!";
        }
    }

	void cleanup()
	{
		auto error = FT_Done_FreeType(library);
	}

    template <typename FontType>
    auto LoadFont(FontSize fontSize, size_t DPI, const char* fontPath = FontType::path)
    {
        auto fontID = getFontID<FontType>();

        auto& fontData = fontMaps[fontID];
        FT_Face& face = fontData.face;
        auto newFaceError = FT_New_Face(library, fontPath, 0, &face);
        auto setSizeError = FT_Set_Char_Size(face, gsl::narrow_cast<FT_F26Dot6>(0), gsl::narrow_cast<FT_F26Dot6>(fontSize * 64), gsl::narrow<FT_UInt>(DPI), gsl::narrow<FT_UInt>(0U));

        auto& glyphMapForSize = fontData.glyphsBySize[fontSize];

        FT_UInt glyphID;
        FT_ULong charcode;
        
        charcode = FT_Get_First_Char(face, &glyphID);

        while (glyphID)
        {
            // load and render glyph
            auto loadError = FT_Load_Glyph(face, glyphID, FT_LOAD_RENDER);
			FT_Glyph glyphRawPtr;
			auto getError = FT_Get_Glyph(face->glyph, &glyphRawPtr);
            GlyphPtr glyphPtr(&glyphRawPtr);
			glyphMapForSize.insert(std::make_pair(charcode, std::move(glyphPtr)));

			charcode = FT_Get_Next_Char(face, charcode, &glyphID);
        }

		return std::make_tuple(glyphMapForSize.cbegin(), glyphMapForSize.cend());
    }

	Bitmap getFullBitmap(FT_Glyph glyph)
	{
		auto bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);
		auto& sourceBitmap = bitmapGlyph->bitmap;

		struct RGBA
		{
			uint8_t r,g,b,a;
			RGBA()
			{}

			RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) :
				r(r), g(g), b(b), a(a)
			{}
		};

		std::vector<RGBA> pixels;
		for (size_t sourceRow = 0; sourceRow < sourceBitmap.rows; sourceRow++)
		{
			for (size_t sourceColumn = 0; sourceColumn < sourceBitmap.width; sourceColumn++)
			{
				size_t sourcePosition = (sourceRow * sourceBitmap.pitch) + sourceColumn;

				auto source = sourceBitmap.buffer;
				pixels.emplace_back(255, 255, 255, source[sourcePosition]);
			}
		}
		FT_BBox box;
		FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_PIXELS, &box);

		Bitmap resultBitmap = Bitmap(
			reinterpret_cast<stbi_uc*>(pixels.data()), 
			sourceBitmap.width, 
			sourceBitmap.rows, 
			std::abs(box.xMin),
			box.yMax,
			sourceBitmap.width * sourceBitmap.rows * sizeof(RGBA)
		);
		return resultBitmap;
	}


}; // class TextEngine
} // namespace Text
