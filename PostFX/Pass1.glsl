#version 120

uniform sampler2D tex;
uniform float pfGamma;
uniform vec2 texel;

void main()
{
	vec3 gamma = vec3(2.2);
	vec3 color = pow(texture2D(tex,gl_TexCoord[0].st).rgb,gamma) +
				 pow(texture2D(tex,gl_TexCoord[0].st+vec2(0,texel.y)).rgb,gamma) +
				 pow(texture2D(tex,gl_TexCoord[0].st+vec2(texel.x,0)).rgb,gamma) +
				 pow(texture2D(tex,gl_TexCoord[0].st+texel).rgb,gamma);
				 
	color /= 4.0;

	gl_FragColor = vec4(pow(color, vec3(pfGamma/2.2)), 1.0); 
}
