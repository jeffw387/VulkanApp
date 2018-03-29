#pragma once

#include "entt.hpp"
#ifndef CONTENTROOT
#define CONTENTROOT
#endif

namespace Sprites
{

/*[[[cog
import cog
import json

with open('Content/SpriteSheets/spritesheet.json') as spritesheetfile:
    spritesheetdict = json.load(spritesheetfile)
    frames = spritesheetdict["frames"]
    for frame in frames:
        cog.outl("    constexpr auto {0} = entt::HashedString(\"{1}\");".format(frame["filename"].replace(".", "_"), frame["filename"]))

]]]*/
constexpr auto star_png = entt::HashedString("star.png");
constexpr auto texture_jpg = entt::HashedString("texture.jpg");
//[[[end]]]

}