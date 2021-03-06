#version 120

uniform sampler2D tex;
uniform sampler2D tex2;
uniform vec2 ssao_max_blur_amount;
//uniform float amount;
void main()
{
	vec4 gamma = vec4(2.2, 2.2, 2.2, 1);
	vec4 dst = pow(texture2D(tex,gl_TexCoord[0].st), gamma);
	vec4 src = pow(texture2D(tex2,gl_TexCoord[0].st), gamma) * vec4(ssao_max_blur_amount.y, ssao_max_blur_amount.y, ssao_max_blur_amount.y, 1);

	dst = clamp((src + dst) - (src * dst), 0.0, 1.0);

	float lum = dot(dst.rgb,  vec3( 0.2125, 0.7154, 0.0721 ));

	vec3 s = vec3(1.0-clamp(src.a, 0.0, ssao_max_blur_amount.x));
	vec3 col = mix(s * dst.rgb, dst.rgb, lum * lum);

	gl_FragColor = vec4(pow(col, vec3(1/2.2)), 1.0);	
}