#pragma once
#include "ft2build.h"
#include FT_FREETYPE_H

class TextEngine
{
    FT_Library library;

    void Init()
    {
        auto error = FT_Init_FreeType(&library);
    }
};

