#pragma once

#include "entt.hpp"
#include "vka/Sprite.hpp"
#ifndef CONTENTROOT
#define CONTENTROOT
#endif

namespace Sprites
{


/*[[[cog
import cog
import json
cog.outl("    struct SpriteSheet1")
cog.outl("    {")
jsonPath = "Content/SpriteSheets/spritesheet.json"
imagePath = "Content/SpriteSheets/spritesheet.png"
cog.outl("        static constexpr auto ImagePath = entt::HashedString(CONTENTROOT \"{0}\");".format(imagePath))
with open(jsonPath) as spritesheetfile:
    spritesheetdict = json.load(spritesheetfile)
    frames = spritesheetdict["frames"]
    imageWidth = spritesheetdict["meta"]["size"]["w"]
    imageHeight = spritesheetdict["meta"]["size"]["h"]
    for frameStruct in frames:
        frame = frameStruct["frame"]
        structName = frameStruct["filename"].replace(".", "")
        fileName = frameStruct["filename"]
        left = frame["x"]
        top = frame["y"]
        right = left + frame["w"]
        bottom = top + frame["h"]
        leftUV = left / imageWidth
        topUV = top / imageHeight
        rightUV = right / imageWidth
        bottomUV = bottom / imageHeight
        cog.outl("        struct {0}".format(structName))
        cog.outl("        {")
        cog.outl("            static constexpr auto Name = entt::HashedString(\"{0}\");".format(fileName))
        cog.outl("            static inline vka::Quad SpriteQuad = vka::MakeQuad({0}, {1}, {2}, {3}, {4}, {5}, {6}, {7});".format(left, top, right, bottom, leftUV, topUV, rightUV, bottomUV))
        #cog.outl("            static constexpr auto Left = {0};".format(left))
        #cog.outl("            static constexpr auto Top = {0};".format(top))
        #cog.outl("            static constexpr auto Right = {0};".format(right))
        #cog.outl("            static constexpr auto Bottom = {0};".format(bottom))
        #cog.outl("            static constexpr auto LeftUV = {0};".format(leftUV))
        #cog.outl("            static constexpr auto TopUV = {0};".format(topUV))
        #cog.outl("            static constexpr auto RightUV = {0};".format(rightUV))
        #cog.outl("            static constexpr auto BottomUV = {0};".format(bottomUV))
        cog.outl("        };")
    cog.outl("    };")

]]]*/
struct SpriteSheet1
{
    static constexpr auto ImagePath = entt::HashedString(CONTENTROOT "Content/SpriteSheets/spritesheet.png");
    struct starpng
    {
        static constexpr auto Name = entt::HashedString("star.png");
        static inline vka::Quad SpriteQuad = vka::MakeQuad(0, 0, 512, 512, 0.0, 0.0, 0.5, 1.0);
    };
    struct texturejpg
    {
        static constexpr auto Name = entt::HashedString("texture.jpg");
        static inline vka::Quad SpriteQuad = vka::MakeQuad(512, 0, 1024, 512, 0.5, 0.0, 1.0, 1.0);
    };
};
//[[[end]]]

}