#pragma once
#include "ft2build.h"
#include FT_FREETYPE_H
#include <iostream>
#include "VulkanApp.h"
#include "MPL.hpp"
#include <utility>

using FontID = size_t;
using FontSize = size_t;

template <typename... fontTypes>
class TextEngine
{
    std::map<FontID, std::map<FontSize

public:

    template <typename FontType>
    auto getFontID()
    {
        return static_cast<FontID>(MPL::IndexOf<FontType, fontTypes...>::value);
    }

    void Init()
    {
        auto error = FT_Init_FreeType(&library);
        if (error != 0)
        {
            std::cerr << "Error initializing Freetype!";
        }

        FT_UInt glyphID;
        glyphIndices.push_back(FT_Get_First_Char(face, &glyphID));

        while (glyphID)
        {
            glyphIndices.push_back(FT_Get_Next_Char(face, glyphIndices.back(), &glyphID));
        }
    }

    template <size_t ID, size_t size>
    void loadFace(const char* fontPath, FT_Long faceIndex)
    {
        FT_Face face;
        
        auto error = FT_New_Face(library, fontPath, faceIndex, &face);
        if (error == FT_Err_Unknown_File_Format)
        {
            std::cerr << "Error loading font AeroviasBrasil, font format not supported.\r\n";
        }
        else if (error)
        {
            std::cerr << "Unknown error while loading font AeroviasBrasil.\r\n";
        }
    }

    void setFaceSize()
    {
        FT_Set_Char_Size(face, 0, 12*64, 166, 166);
    }
};

