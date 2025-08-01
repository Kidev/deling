#version 440

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec2 a_texcoord;

layout(location = 0) out vec4 v_color;
layout(location = 1) out vec2 v_texcoord;

layout(std140, binding = 0) uniform buf {
    mat4 modelMatrix;
    mat4 projectionMatrix;
    mat4 viewMatrix;
    float pointSize;
};

void main()
{
    mat4 mat = projectionMatrix * viewMatrix * modelMatrix;

    gl_Position = mat * a_position;
    gl_PointSize = pointSize;
    v_color = a_color;
    v_texcoord = a_texcoord;
}
