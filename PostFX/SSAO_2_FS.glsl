#version 120

uniform sampler2D tex;
uniform sampler2D tex2;
uniform float ssao_max;
void main()
{
	vec3 dst = pow(texture2D(tex,gl_TexCoord[0].st).rgb, vec3(2.2));
	float lum = dot(dst,  vec3( 0.2125, 0.7154, 0.0721 ));
	vec3 src = vec3(1.0)-clamp(texture2D(tex2,gl_TexCoord[0].st).rgb,vec3(0.0), vec3(ssao_max));
	vec3 col = mix(src * dst, dst, lum * lum);
	gl_FragColor = vec4(pow(col, vec3(1/2.2)), 1.0);
}