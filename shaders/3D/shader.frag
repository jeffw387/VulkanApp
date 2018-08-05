#version 450
#extension GL_ARB_separate_shader_objects : enable
// ---Fragment Shader 3D pipeline---

// --specialization constants--
layout(constant_id = 0) const uint MaterialCount = 1;
layout(constant_id = 2) const uint LightCount = 3;

// --uniform buffers--
// per material
layout(set = 0, binding = 2) uniform Materials
{
    vec4 color;
} materials[MaterialCount];

// per frame
layout(set = 0, binding = 4) uniform Lights
{
    vec4 position_WorldSpace;
    vec4 color;
} lights[LightCount];

// push constants
layout(push_constant) uniform PushConstants
{
    layout(offset = 0) uint materialIndex;
	layout (offset = 4) uint textureIndex;
} push;

// --shader interface--
layout(location = 0) in vec3 normal_CameraSpace;
layout(location = 1) in vec3 lightDirection_CameraSpace[LightCount];

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 diffuseColor = materials[push.materialIndex].color;
    for (uint i = 0; i < LightCount; i++)
    {
        vec3 l = normalize(lightDirection_CameraSpace[i]);
        vec3 n = normalize(normal_CameraSpace);
        float cosTheta = clamp(dot(n, l), 0, 1);
        diffuseColor *= vec4(lights[i].color.rgb * (cosTheta * lights[i].color.a), 1);
    }
    outColor = diffuseColor;
}