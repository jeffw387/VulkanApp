#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

layout(constant_id = 0) const uint TextureCount = 1;

// static (not updated after load)
layout(set = 0, binding = 0) uniform sampler samp;
layout(set = 0, binding = 1) uniform texture2D tex[TextureCount];

// updated per model
layout(set = 2, binding = 0) uniform ModelData
{
    vec4 color;
    uint imageOffset;
} modelData;

layout(location = 0) in vec2 texUV;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 sampledColor = texture(sampler2D(tex[modelData.imageOffset], samp), texUV);
    outColor = modelData.color * sampledColor;
}