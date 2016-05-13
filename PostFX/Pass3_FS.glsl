uniform sampler2D tex;
uniform sampler2D tex2;
uniform float amount;

void main()
{
	vec3 gamma = vec3(2.2);
	vec3 dst = pow(texture2D(tex,gl_TexCoord[0].st).rgb,gamma);
	vec3 src = pow(texture2D(tex2,gl_TexCoord[0].st).rgb,gamma)*amount;
	gl_FragColor = vec4(pow(clamp((src + dst) - (src * dst), 0.0, 1.0), vec3(1/2.2)),1);
}