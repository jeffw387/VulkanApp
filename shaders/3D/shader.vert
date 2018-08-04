#version 450
#extension GL_ARB_separate_shader_objects : enable
// ---Vertex Shader 3D pipeline---

// --specialization constants--
layout(constant_id = 2) const uint LightCount = 3;

// --vertex attributes--
layout(location = 0) in vec3 vertexPosition_ModelSpace;
layout(location = 1) in vec3 vertexNormal_ModelSpace;

// --uniform buffers--
// per frame
layout(set = 0, binding = 3) uniform Camera
{
    mat4 view;
    mat4 projection;
} camera;
layout(set = 0, binding = 4) uniform Lights
{
    vec4 position_WorldSpace;
    vec4 color;
} lights[LightCount];

// per draw
layout(set = 1, binding = 0) uniform Instance
{
    mat4 M;
    mat4 MVP;
} instance;

// --shader interface--
layout(location = 0) out vec3 vertexNormal_CameraSpace;
layout(location = 1) out vec3 lightDirection_CameraSpace[LightCount];
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

    for (uint i = 0; i < LightCount; i++)
    {
        vec3 lightPosition_CameraSpace = (V * vec4(lights[i].position_WorldSpace)).xyz;
        lightDirection_CameraSpace[i] = lightPosition_CameraSpace + eyePosition_CameraSpace;
    }

    vertexNormal_CameraSpace = (V * M * vec4(vertexNormal_ModelSpace, 0)).xyz;
}