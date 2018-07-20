#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(constant_id = 0) const uint MaxLights = 3;

layout(location = 0) in vec3 vertexPosition_ModelSpace;
layout(location = 1) in vec3 vertexNormal_ModelSpace;

// per draw
layout(set = 2, binding = 0) uniform Instance
{
    mat4 M;
    mat4 MVP;
} instance;

// per frame
layout(set = 1, binding = 0) uniform Camera
{
    mat4 view;
    mat4 projection;
} camera;
layout(set = 1, binding = 1) uniform Lights
{
    vec4 position_WorldSpace;
    vec4 color;
} lights[MaxLights];

layout(location = 0) out vec3 vertexNormal_CameraSpace;
out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    mat4 MVP = instance.MVP;
    mat4 M = instance.M;
    mat4 V = camera.view;
    mat4 P = camera.projection;

    gl_Position = MVP * vec4(vertexPosition_ModelSpace, 1.0);
    
    vec3 vertexPosition_CameraSpace = (V * M * vec4(vertexPosition_ModelSpace, 1)).xyz;
    vec3 eyePosition_CameraSpace = vec3(0, 0, 0) - vertexPosition_CameraSpace;

    for (uint i = 0; i < MaxLights; i++)
    {
        vec3 lightPosition_CameraSpace = (V * vec4(lights.position_WorldSpace[i])).xyz;
        lightDirection_CameraSpace[i] = lightPosition_CameraSpace + eyePosition_CameraSpace;
    }

    vertexNormal_CameraSpace = (V * M * vec4(vertexNormal_ModelSpace, 0)).xyz;
}