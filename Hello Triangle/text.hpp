#pragma once
#include "ft2build.h"
#include FT_FREETYPE_H
#include <iostream>
#include "VulkanApp.h"
#include "MPL.hpp"
#include <utility>

using FontID = size_t;
using FontSize = size_t;

struct FontData
{
    FT_Face face;
    std::map<FontSize, std::map<char, 
};

template <typename... fontTypes>
class TextEngine
{
    std::map<FontID, std::map<FontSize, std::map<char, vke::Image2D>>> fontMaps;

public:

    template <typename FontType>
    auto getFontID()
    {
        return static_cast<FontID>(MPL::IndexOf<FontType, fontTypes...>::value);
    }

    void Init(std::vector<std::string> fontPaths, vke::LogicalDevice& device)
    {
        auto error = FT_Init_FreeType(&library);
        if (error != 0)
        {
            std::cerr << "Error initializing Freetype!";
        }

        FontID currentFont = 0;

        for (auto& fontPair : fontMaps)
        {
            auto fontPath = fontPaths[currentFont];
            for (auto& sizePair : fontPair.second)
            {
                FT_UInt glyphID;
                glyphIndices.push_back(FT_Get_First_Char(face, &glyphID));

                while (glyphID)
                {
                    glyphIndices.push_back(FT_Get_Next_Char(face, glyphIndices.back(), &glyphID));
                }
            }
        }
        
    }
};

