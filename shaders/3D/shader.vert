#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vertexPosition_ModelSpace;
layout(location = 1) in vec3 vertexNormal_ModelSpace;

layout(binding = 0) uniform DynamicMatrices
{
    mat4 M;
    mat4 MVP;
} dynamicMatrices;

layout(binding = 1) uniform Matrices
{
    mat4 view;
    mat4 projection;
} matrices;

layout(constant_id = 0) const uint MaxLights = 3;
layout(binding = 2) uniform Lights
{
    vec4 position_WorldSpace[MaxLights];
    vec4 color[MaxLights];
} lights;

layout(location = 0) out vec3 vertexNormal_CameraSpace;
layout(location = 1) out vec3 lightDirection_CameraSpace[MaxLights];
layout(location = 4) flat out vec4 lightColors[MaxLights];
out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    mat4 MVP = dynamicMatrices.MVP;
    mat4 M = dynamicMatrices.M;
    mat4 V = matrices.view;
    mat4 P = matrices.projection;

    gl_Position = MVP * vec4(vertexPosition_ModelSpace, 1.0);
    
    vec3 vertexPosition_CameraSpace = (V * M * vec4(vertexPosition_ModelSpace, 1)).xyz;
    vec3 eyeDirection_CameraSpace = vec3(0, 0, 0) - vertexPosition_CameraSpace;

    for (uint i = 0; i < MaxLights; i++)
    {
        vec3 lightPosition_CameraSpace = (V * vec4(lights.position_WorldSpace[i])).xyz;
        lightDirection_CameraSpace[i] = lightPosition_CameraSpace + eyeDirection_CameraSpace;
        lightColors[i] = lights.color[i];
    }

    vertexNormal_CameraSpace = (V * M * vec4(vertexNormal_ModelSpace, 0)).xyz;
}