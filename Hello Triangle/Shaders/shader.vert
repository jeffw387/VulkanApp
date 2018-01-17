#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform VertexUniforms
{
    mat4 transform;
} uniforms;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;
out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    gl_Position = uniforms.transform * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
}