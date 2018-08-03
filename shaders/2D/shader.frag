#version 450
#extension GL_ARB_separate_shader_objects : enable
// ---Fragment Shader 2D pipeline---

// --specialization constants--
layout(constant_id = 0) const uint MaterialCount = 1;
layout(constant_id = 1) const uint ImageCount = 1;

// --uniform buffers--
// static
layout(set = 0, binding = 0) uniform sampler samp;
layout(set = 0, binding = 1) uniform texture2D tex[ImageCount];
layout(set = 0, binding = 2) uniform Materials
{
    vec4 color;
} materials[MaterialCount];

// push constants
layout(push_constant) uniform PushConstants
{
    layout (offset = 0) uint materialIndex;
	layout (offset = 4) uint textureIndex;
} push;

// --shader interface--
layout(location = 0) in vec2 texUV;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 sampledColor = texture(sampler2D(tex[push.textureIndex], samp), texUV);
    outColor = materials[push.materialIndex].color * sampledColor;
}