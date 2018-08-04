#version 450
#extension GL_ARB_separate_shader_objects : enable
// ---Vertex Shader 2D pipeline---

// --vertex attributes--
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

// --uniform buffers--
// per draw update
layout(set = 1, binding = 0) uniform Instance
{
    mat4 M;
    mat4 MVP;
} instance;

// --shader interface--
layout(location = 0) out vec2 outTexCoord;
out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    outTexCoord = inTexCoord;

    gl_Position = instance.MVP * vec4(inPosition, 0.0, 1.0);
}