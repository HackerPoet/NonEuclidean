#version 150

uniform mat4 mvp;
uniform mat4 model;

in vec3 in_pos;
in vec2 in_uv;

out vec3 ex_worldPos;

void main(void) {
    gl_Position = mvp * vec4(in_pos, 1.0);
    ex_worldPos = (model * vec4(in_pos, 1.0)).xyz;
}
