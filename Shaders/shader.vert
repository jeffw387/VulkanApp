#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstants
{
    uint imageIndex;
    vec4 color;
    mat4 mvp;
} pushConstants;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) flat out uint outImageOffset;
layout(location = 2) flat out vec4 outColor;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    outTexCoord = inTexCoord;
    outImageOffset = pushConstants.imageIndex;
    outColor = pushConstants.color;

    gl_Position = pushConstants.mvp * vec4(inPosition, 0.0, 1.0);
}