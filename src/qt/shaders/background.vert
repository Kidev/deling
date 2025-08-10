#version 450

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUv;

layout(location = 0) out vec2 vUv;

void main()
{
    vUv = inUv;
    gl_Position = vec4(inPos.xy, 0.0, 1.0);
}
