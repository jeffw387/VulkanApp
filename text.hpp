#pragma once

#include <iostream>
#include <utility>
#include "Bitmap.hpp"
#include "Sprite.hpp"
#include <memory>
#include <gsl/gsl>
#include <optional>
#include "Texture2D.hpp"
#include <map>
#include "UniqueHandle.hpp"
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"

//namespace Text
//{
//
//
//class Engine
//{
//public:
//
//	UniqueFTLibrary m_Library;
//    std::map<FontID, FontData> m_FontMaps;
//
//	Engine()
//    {
//		FT_Library temp;
//        auto error = FT_Init_FreeType(&temp);
//        if (error != 0)
//        {
//            std::cerr << "Error initializing FreeType.\n";
//        }
//		else
//		{
//			std::cout << "Initialized FreeType.\n";
//		}
//		m_Library = std::move(temp);
//    }
//
//
//
//
//	
//}; // class Engine
//} // namespace Text
