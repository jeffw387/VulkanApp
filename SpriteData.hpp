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
        spriteWidth = frame["w"]
        spriteHeight = frame["h"]
        right = left + spriteWidth
        bottom = top + spriteHeight
        pivotX = spriteWidth * 0.5
        pivotY = spriteHeight * 0.5
        vertLeft = -pivotX
        vertRight = pivotX
        vertTop = -pivotY
        vertBottom = pivotY
        leftUV = left / imageWidth
        topUV = top / imageHeight
        rightUV = right / imageWidth
        bottomUV = bottom / imageHeight
        cog.outl("        struct {0}".format(structName))
        cog.outl("        {")
        cog.outl("            static constexpr auto Name = entt::HashedString(\"{0}\");".format(fileName))
        cog.outl("            static inline vka::Quad SpriteQuad = vka::MakeQuad({0}, {1}, {2}, {3}, {4}, {5}, {6}, {7});".format(vertLeft, vertTop, vertRight, vertBottom, leftUV, topUV, rightUV, bottomUV))
        cog.outl("        };")
    cog.outl("    };")

]]]*/
struct SpriteSheet1
{
    static constexpr auto ImagePath = entt::HashedString(CONTENTROOT "Content/SpriteSheets/spritesheet.png");
    struct starpng
    {
        static constexpr auto Name = entt::HashedString("star.png");
        static inline vka::Quad SpriteQuad = vka::MakeQuad(-256.0, -256.0, 256.0, 256.0, 0.0, 0.0, 0.5, 1.0);
    };
    struct texturejpg
    {
        static constexpr auto Name = entt::HashedString("texture.jpg");
        static inline vka::Quad SpriteQuad = vka::MakeQuad(-256.0, -256.0, 256.0, 256.0, 0.5, 0.0, 1.0, 1.0);
    };
};
//[[[end]]]

}