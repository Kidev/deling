#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 vColor;

layout(std140, binding = 0) uniform Ubo {
    mat4 mvp;
};

void main()
{
    vColor = inColor;
    gl_Position = mvp * vec4(inPosition, 1.0);
}
