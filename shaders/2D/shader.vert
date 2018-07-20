#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

// vertex attributes
layout(binding = 0, location = 0) in vec2 inPosition;
layout(binding = 0, location = 1) in vec2 inTexCoord;

// uniform buffers
// per frame update
layout(set = 1, binding = 0) uniform Matrices
{
    mat4 view;
    mat4 projection;
} matrices;

// per draw update
layout(set = 3, binding = 0) uniform DynamicMatrices
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