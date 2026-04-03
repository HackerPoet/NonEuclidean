#version 150
precision highp float;

uniform vec3 light_pos;
uniform float far_plane;

in vec3 ex_worldPos;

out vec4 fragColor;

void main(void) {
    float dist = length(ex_worldPos - light_pos) / far_plane;
    fragColor = vec4(dist, 0.0, 0.0, 1.0);
}
