#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(constant_id = 0) const uint TextureCount = 1;

layout(set = 0, binding = 0) uniform sampler samp;
layout(set = 0, binding = 1) uniform texture2D tex[TextureCount];

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) flat in uint inImageOffset;
layout(location = 2) flat in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 sampledColor = texture(sampler2D(tex[inImageOffset], samp), inTexCoord);
    outColor = inColor * sampledColor;
}