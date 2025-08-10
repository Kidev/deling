#version 450

layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outFragColor;

layout(binding = 1) uniform sampler2D uTex;

void main()
{
    vec4 t = texture(uTex, vUv);
    outFragColor = vec4(t.b, t.g, t.r, t.a);
}
