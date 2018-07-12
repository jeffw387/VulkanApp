#version 450
#extension GL_ARB_separate_shader_objects : enable

// vertex attributes
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

// uniform buffers
layout(binding = 0) uniform Matrices
{
    mat4 view;
    mat4 projection;
} matrices;

layout(binding = 1) uniform DynamicMatrices
{
    mat4 M;
    mat4 MVP;
} dynamicMatrices;

// shader interface
layout(location = 0) out vec2 outTexCoord;
out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    outTexCoord = inTexCoord;

    gl_Position = dynamicMatrices.MVP * vec4(inPosition, 0.0, 1.0);
}