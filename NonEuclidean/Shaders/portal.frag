#version 150
precision highp float;

//Inputs
uniform sampler2D tex;
in vec4 ex_uv;

//Outputs
out vec4 gl_FragColor;

void main(void) {
	vec2 uv = (ex_uv.xy / ex_uv.w);
	uv = uv*0.5 + 0.5;
	gl_FragColor = vec4(texture2D(tex, uv).rgb, 1.0);
}
