#version 120
#extension GL_ARB_texture_rectangle : enable

uniform vec4 Local0;
uniform vec4 Local1;
uniform vec4 Local2;
uniform sampler2DRect Texture0;
uniform int rings;

float read_depth_2( vec2 coord )
{
    return smoothstep(0.45, 0.55, texture2DRect( Texture0, coord ).r);
}

float compare_depth_2( float depth1, float depth2 )
{
	float aoRange = Local1.w;
	float depthDiff = depth1 - depth2;
	float propagation = depthDiff;
	float rangeDiff = sqrt(clamp( 1.0 - propagation / aoRange, 0.0, 1.0 ));
	float ao = Local1.z * depthDiff * (2.0 - depthDiff) * rangeDiff;
	return ao;
}

float read_depth( vec2 coord )
{
	return Local0.x / (Local0.y + Local0.x - texture2DRect( Texture0, coord ).r * Local0.y);
}

vec2 rand( vec2 coord )
{
	float coordDot = dot( coord, vec2( 12.9898, 78.233 ) );
	float noiseX = fract( sin( coordDot ) * 43758.5453 );
	float noiseY = fract( sin( coordDot * 2.0 ) * 43758.5453 );
	return vec2( noiseX, noiseY ) * 0.001;
}

float compare_depth( float depth1, float depth2 )
{
	float aoRange = Local1.w;
	float depthDiff = depth1 - depth2;
	float propagation = depthDiff * (Local0.y - Local0.x);
	float rangeDiff = sqrt(clamp( 1.0 - propagation / aoRange, 0.0, 1.0 ));
	float ao = Local1.z * depthDiff * (2.0 - depthDiff) * rangeDiff;
	return ao;
}

void main(void)
{
	float depth = read_depth_2( gl_TexCoord[0].xy );
	float ao = 0;
	if ( depth > Local1.x && depth < Local1.y ) {
		vec2 noise = rand( gl_TexCoord[0].zw );
		float distScale = 1.0 + 0.01 * Local0.z * sqrt( 1.0 - depth );
		float w = distScale + (noise.x*(1.0-noise.x)) * Local0.z;

		vec2 ofs;
		float d;
		int samples = 3;
		float TWO_PI = 2.0 * 3.14159265;
		float ang = noise.y * 1000.0 * TWO_PI;
		float sc = 3.0;
		for ( int i = 1 ; i <= rings; i++ ) {
			float angleStep = TWO_PI / float( samples * i );
			sc = sc / 3.0;
			for ( int j = 1 ; j <= samples*i; j++ ) {
			    ang = ang + angleStep;
				ofs.x = cos( ang ) * float(i) * w;
				ofs.y = sin( ang ) * float(i) * w;
				d = read_depth_2( gl_TexCoord[0].xy + ofs );

				// ignore occluder with too low depth value (possible viewmodel)
				ao += compare_depth_2( depth, d ) * sc;
			}
		}
	}

	gl_FragColor = vec4(ao, ao, ao, 1);
}