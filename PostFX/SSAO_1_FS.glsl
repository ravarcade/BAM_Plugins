#version 120
#extension GL_ARB_texture_rectangle : enable

uniform sampler2D Texture0;
uniform sampler2D Texture1;
uniform int rings;
uniform vec4 pixelSize; // and depth conversion params
uniform vec4 aoRangeLevelAspect; // and aspe
uniform float gamma;
//uniform vec2 texel;


// deph: 1 - far, 0 - near,
float read_depth( vec2 coord )
{
	return pixelSize.x / (pixelSize.y + pixelSize.x - texture2D( Texture1, coord ).r * pixelSize.y);
}

// random generator output vec2: x = <0,1), x = <0,1)
vec2 rand( vec2 coord )
{
	float coordDot = dot( coord, vec2( 12.9898, 78.233 ) );
	float noiseX = fract( sin( coordDot ) * 43758.5453 );
	float noiseY = fract( sin( coordDot * 2.0 ) * 43758.5453 );
	return vec2( noiseX, noiseY );
}

float compare_depth( float depth1, float depth2 )
{
	float aoRange = aoRangeLevelAspect.y;
	float depthDiff = depth1 - depth2;
	float propagation = depthDiff * (pixelSize.y - pixelSize.x);
	float rangeDiff = sqrt(clamp( 1.0 - propagation / aoRange, 0.0, 1.0 ));
	float ao = aoRangeLevelAspect.w * depthDiff * (2.0 - depthDiff) * rangeDiff;
	return ao;
}

void main(void)
{
	float depth = read_depth( gl_TexCoord[0].xy );
	float ao = 0.0;

	vec2 noise = rand( gl_TexCoord[0].xy ); // 2 random values in range <0,1)
	float w = (noise.x +  sqrt(1.0 - depth)) * aoRangeLevelAspect.x; // w = 0 ... 1.25 * aoRange = 0 ... 0.018 (1.8%)
	vec2 radius = w * vec2(1, aoRangeLevelAspect.z);

	vec2 ofs;
	float d;
	int samples = 3;
	float TWO_PI = 2.0 * 3.14159265;
	float ang = noise.y * 1000.0 * TWO_PI;
	float sc = 1;
	for ( int i = 1 ; i <= rings; i++ ) {
		float angleStep = TWO_PI / float( samples * i );
		vec2 ofsscale = radius * i;
		sc = sc / samples;
		for ( int j = 1 ; j <= samples*i; j++ ) {
		    ang = ang + angleStep;
			ofs = vec2(cos(ang), sin(ang)) * ofsscale;

			d = read_depth( gl_TexCoord[0].xy + ofs );

//			// ignore occluder with too low depth value (possible viewmodel)
//			if ( d < Local1.x ) continue;
			ao += compare_depth( depth, d ) * sc;
		}
	}

	vec3 color = pow(texture2D( Texture0, gl_TexCoord[0].xy ).rgb, vec3(2.2));

	gl_FragColor = vec4(pow(color, vec3(gamma/2.2)), ao);
}