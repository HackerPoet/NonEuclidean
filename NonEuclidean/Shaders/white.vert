#version 150

uniform mat4 mvp;

in vec3 in_pos;
in vec2 in_uv;

void main(void) {
    gl_Position = mvp * vec4(in_pos, 1.0);
}
