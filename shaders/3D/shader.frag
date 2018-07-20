#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(constant_id = 0) const uint MaxLights = 3;
layout(constant_id = 1) const uint MaterialCount = 1;

// per frame
layout(set = 1, binding = 1) uniform Lights
{
    vec4 position_WorldSpace;
    vec4 color;
} lights[MaxLights];

// per material
layout(set = 0, binding = 0) uniform Materials
{
    vec4 color;
} materials[MaterialCount];

layout(push_constant) uniform PushConstants
{
    uint materialIndex;
} pushConstants;

layout(location = 0) in vec3 normal_CameraSpace;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 diffuseColor = materials[materialIndex].color;
    for (uint i = 0; i < MaxLights; i++)
    {
        vec3 l = normalize(lightDirection_CameraSpace[i]);
        vec3 n = normalize(normal_CameraSpace);
        float cosTheta = clamp(dot(n, l), 0, 1);
        diffuseColor *= vec4(lights[i].color.rgb * (cosTheta * lights[i].color.a), 1);
    }
    outColor = diffuseColor;
}