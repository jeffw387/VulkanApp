#pragma once
#include "ft2build.h"
#include FT_FREETYPE_H
#include <iostream>
#include "VulkanApp.h"
#include "MPL.hpp"
#include <utility>

using FontID = size_t;
using FontSize = size_t;
using GlyphID = FT_UInt;
using CharCode = FT_ULong;

struct GlyphData
{
    FT_GlyphSlot glyph;
};

using GlyphMap = std::map<CharCode, GlyphData>;

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
    void Init()
    {
        auto error = FT_Init_FreeType(&library);
        if (error != 0)
        {
            std::cerr << "Error initializing Freetype!";
        }
    }

    template <typename FontType>
    void LoadFont(std::string fontPath, FontSize fontSize, size_t hDPI, size_t vDPI, VkDevice device, vke::Allocator* allocator, VkCommandPool pool, VkQueue queue)
    {
        auto fontID = getFontID<FontType>();

        auto& fontData = fontMaps[fontID];
        FT_Face& face = fontData.face;
        auto newFaceError = FT_New_Face(library, fontPath.c_str(), 0, &face);
        auto setSizeError = FT_Set_Char_Size(face, 0, fontSize * 64, hDPI, vDPI);

        auto& glyphMapForSize = fontData.glyphsBySize[fontSize];

        FT_UInt glyphID;
        FT_ULong charcode;
        
        charcode = FT_Get_First_Char(face, &glyphID);

        while (glyphID)
        {
            // load and render glyph
            auto loadError = FT_Load_Glyph(face, glyphID, FT_LOAD_RENDER);

			auto& glyph = face->glyph;
            auto& bitmap = glyph->bitmap;

            // create a VkImage and store it with the charcode
            GlyphData glyphData = {};
            glyphData.code = charcode;

			// create a staging buffer for the bitmap data
			vke::Buffer stagingBuffer;
			VkDeviceSize bitmapSize = bitmap.width * bitmap.rows;
			stagingBuffer.init(device, allocator, bitmapSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			// map the staging buffer and copy the bitmap into it (tightly packed)
			stagingBuffer.mapMemoryRange();
			for (size_t sourceRow = 0; sourceRow < bitmap.rows; sourceRow++)
			{
				for (size_t sourceColumn = 0; sourceColumn < bitmap.width; sourceColumn++)
				{
					size_t sourcePosition = (sourceRow * bitmap.pitch) + sourceColumn;
					size_t destPosition = (sourceRow * bitmap.width) + sourceColumn;

					auto destination = static_cast<unsigned char*>(stagingBuffer.mappedData);
					auto source = bitmap.buffer;

					destination[destPosition] = source[sourcePosition];
				}
			}
			stagingBuffer.unmap();

			VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            glyphData.image.init(device, allocator, 
				bitmap.width, 
				bitmap.rows, 
				VK_FORMAT_R8_UINT,
				VK_IMAGE_TILING_OPTIMAL, 
				usageFlags,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_SHARING_MODE_EXCLUSIVE,
				VK_IMAGE_LAYOUT_PREINITIALIZED,
				vke::AllocationStyle::NoPreference);

			// copy from staging buffer to device buffer
			glyphData.image.copyFromBuffer(static_cast<VkBuffer>(stagingBuffer), pool, queue);

			FT_BBox cbox;
			FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_PIXELS, &cbox);
			auto xMin = static_cast<float>(cbox.xMin / 64);
			auto yMin = static_cast<float>(cbox.yMin / 64);
			auto xMax = static_cast<float>(cbox.xMax / 64);
			auto yMax = static_cast<float>(cbox.yMax / 64);
			glyphData.TopLeft = glm::vec3(xMin, yMax, 0.0f);
			glyphData.BottomLeft = glm::vec3(xMin, yMin, 0.0f);
			glyphData.BottomRight = glm::vec3(xMax, yMin, 0.0f);
			glyphData.TopRight = glm::vec3(xMax, yMax, 0.0f);

			glyphData.TopLeftUV = glm::vec2(0.0f, 0.0f);
			glyphData.BottomLeftUV = glm::vec2(0.0f, 1.0f);
			glyphData.BottomRightUV = glm::vec2(1.0f, 1.0f);
			glyphData.TopRightUV = glm::vec2(1.0f, 0.0f);
			
            glyphMapForSize[glyphID] = glyphData;
            glyphMapForSize.push_back(FT_Get_Next_Char(face, glyphMapForSize.back(), &glyphID));
        }
    }
};

