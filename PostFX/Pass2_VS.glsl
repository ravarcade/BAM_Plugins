#version 120

#define NUM_BLURAMOUNT 4

attribute vec2 position;
attribute vec2 texture;

uniform vec4 ScaleTexture;
uniform vec4 ScalePosition;

uniform int Orientation; 
uniform int BlurAmount; 
uniform float BlurScale; 
uniform float BlurStrength; 
uniform float TexelSize; 

varying vec2 coords[NUM_BLURAMOUNT+1];
varying float amount[NUM_BLURAMOUNT+1];

float Gaussian (float x, float deviation) { 
	return (1.0 / sqrt(2.0 * 3.141592 * deviation)) * exp(-((x * x) / (2.0 * deviation))); 
} 

void main() {
    vec2 UV		 = vec2(texture.xy * ScaleTexture.xy + ScaleTexture.zw);
	gl_Position	 = vec4(position.xy * ScalePosition.xy + ScalePosition.zw, 0, 1);

	float halfBlur = float(BlurAmount) * 0.5; 
	float deviation = halfBlur * 0.35; 
	deviation *= deviation; 
	float strength = 1.0 - BlurStrength; 

	for ( int i=1; i<BlurAmount; ++i) {
		float offset = float(i) * TexelSize * BlurScale; 
		if (Orientation == 0) {
			coords[i] = vec2(offset, 0.0);
		} else {
			coords[i] = vec2(0.0, offset);
		}
		amount[i] = Gaussian(float(i) * strength, deviation);
	}
	coords[0]= UV;
	amount[0] = Gaussian(0, deviation);
}
