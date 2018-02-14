#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(constant_id = 0) const int TextureCount = 1;

layout(set = 0, binding = 0) uniform sampler samp;
layout(set = 0, binding = 1) uniform texture2D tex[TextureCount];

layout(push_constant) uniform pushBlock
{
    mat4 transform;
    uint textureID;
    float r;
    float g;
    float b;
    float a;
} pushConstants;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 sampledColor = texture(sampler2D(tex[pushConstants.textureID], samp), fragTexCoord);
    vec4 pushColor = vec4(pushConstants.r, pushConstants.g, pushConstants.b, pushConstants.a);
    outColor = pushColor * sampledColor;
}