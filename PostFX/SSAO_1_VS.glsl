#version 120
attribute vec2 position;
attribute vec2 texture;

uniform vec4 ScaleTexture;
uniform vec4 ScalePosition;
uniform vec4 Local0;

void main() {
    gl_TexCoord[0] = vec4(texture.xy * ScaleTexture.xy + ScaleTexture.zw,  1.0 / Local0.z, 1.0 / Local0.w );
//	gl_TexCoord[0] = gl_MultiTexCoord0.xyxy * vec4( 1.0, 1.0, 1.0 / Local0.z, 1.0 / Local0.w );
	gl_Position    = vec4(position.xy * ScalePosition.xy + ScalePosition.zw, 0, 1);
}
