#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(constant_id = 0) const uint MaxLights = 3;

layout(push_constant) uniform FragmentPushConstants
{
    vec4 color;
} fragmentPushConstants;

layout(location = 0) in vec3 normal_CameraSpace;
layout(location = 1) in vec3 lightDirection_CameraSpace[MaxLights];
layout(location = 4) flat in vec4 lightColors[MaxLights];

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 diffuseColor = fragmentPushConstants.color;
    for (uint i = 0; i < MaxLights; i++)
    {
        vec3 l = normalize(lightDirection_CameraSpace[i]);
        vec3 n = normalize(normal_CameraSpace);
        float cosTheta = clamp(dot(n, l), 0, 1);
        diffuseColor *= vec4(lightColors[i].rgb * cosTheta, 1) * lightColors[i].a;
    }
    outColor = diffuseColor;
}