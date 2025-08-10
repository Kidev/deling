#version 440
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(std140, binding = 0) uniform Transform {
    mat4 mvp;
};
layout(location = 0) out vec3 v_color;
void main() {
    v_color = color;
    gl_Position = mvp * vec4(position, 1.0);
}
