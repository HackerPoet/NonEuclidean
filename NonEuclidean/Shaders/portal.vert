#version 150

//Globals
uniform mat4 mvp;

//Inputs
in vec3 in_pos;
in vec2 in_uv;

//Outputs
out vec4 ex_uv;

void main(void) {
	gl_Position = mvp * vec4(in_pos, 1.0);
	ex_uv = gl_Position;
}
