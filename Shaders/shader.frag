#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(constant_id = 0) const uint TextureCount = 1;

layout(set = 0, binding = 0) uniform sampler samp;
layout(set = 0, binding = 1) uniform texture2D tex[TextureCount];

layout(push_constant) uniform PushConstants
{
    mat4 mvp;
    uint imageOffset;
    vec4 color;
} pushConstants;

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 sampledColor = texture(sampler2D(tex[pushConstants.imageOffset], samp), inTexCoord);
    outColor = pushConstants.color * sampledColor;
}