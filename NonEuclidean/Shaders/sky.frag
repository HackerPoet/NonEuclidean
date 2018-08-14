#version 150
precision highp float;

#define LIGHT vec3(0.36, 0.80, 0.48)
#define SUN_SIZE 0.002
#define SUN_SHARPNESS 1.0

//Inputs
in vec3 ex_normal;

//Outputs
out vec4 gl_FragColor;

void main(void) {
	vec3 n = normalize(ex_normal);

	float h = (1.0 - n.y) * (1.0 - n.y) * 0.5;
	vec3 sky = vec3(0.2 + h, 0.5 + h, 1.0);
	
	float s = dot(n, LIGHT) - 1.0 + SUN_SIZE;
	float sun = min(exp(s * SUN_SHARPNESS / SUN_SIZE), 1.0);
	
	gl_FragColor = vec4(max(sky, sun), 1.0);
}
