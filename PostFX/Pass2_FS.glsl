#version 120

#define NUM_BLURAMOUNT 4

uniform sampler2D tex;

varying vec2 coords[NUM_BLURAMOUNT+1];
varying float amount[NUM_BLURAMOUNT+1];

void main()
{
	vec3 color = vec3(0.0);
	vec3 gamma = vec3(2.2);

	color += pow(texture2D(tex, coords[0]-coords[4]).rgb,gamma)*amount[4];
	color += pow(texture2D(tex, coords[0]-coords[3]).rgb,gamma)*amount[3];
	color += pow(texture2D(tex, coords[0]-coords[2]).rgb,gamma)*amount[2];
	color += pow(texture2D(tex, coords[0]-coords[1]).rgb,gamma)*amount[1];
	color += pow(texture2D(tex, coords[0]).rgb,gamma) * amount[0];
	color += pow(texture2D(tex, coords[0]+coords[1]).rgb,gamma)*amount[1];
	color += pow(texture2D(tex, coords[0]+coords[2]).rgb,gamma)*amount[2];
	color += pow(texture2D(tex, coords[0]+coords[3]).rgb,gamma)*amount[3];
	color += pow(texture2D(tex, coords[0]+coords[4]).rgb,gamma)*amount[4];

	gl_FragColor = vec4(pow(color,vec3(1/2.2)),1);
}
