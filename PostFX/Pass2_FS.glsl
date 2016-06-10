#version 120

#define NUM_BLURAMOUNT 4

uniform sampler2D tex;

varying vec2 coords[NUM_BLURAMOUNT+1];
varying float amount[NUM_BLURAMOUNT+1];

void main()
{
	vec4 color = vec4(0.0);
	vec4 gamma = vec4(2.2, 2.2, 2.2, 1);

	color += pow(texture2D(tex, coords[0]-coords[4]),gamma)*amount[4];
	color += pow(texture2D(tex, coords[0]-coords[3]),gamma)*amount[3];
	color += pow(texture2D(tex, coords[0]-coords[2]),gamma)*amount[2];
	color += pow(texture2D(tex, coords[0]-coords[1]),gamma)*amount[1];
	color += pow(texture2D(tex, coords[0]),gamma) * amount[0];
	color += pow(texture2D(tex, coords[0]+coords[1]),gamma)*amount[1];
	color += pow(texture2D(tex, coords[0]+coords[2]),gamma)*amount[2];
	color += pow(texture2D(tex, coords[0]+coords[3]),gamma)*amount[3];
	color += pow(texture2D(tex, coords[0]+coords[4]),gamma)*amount[4];

	gl_FragColor = pow(color,vec4(1/2.2, 1/2.2, 1/2.2, 1));
}
