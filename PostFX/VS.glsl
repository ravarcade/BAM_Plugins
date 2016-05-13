#version 120
attribute vec2 position;
attribute vec2 texture;

uniform vec4 ScaleTexture;
uniform vec4 ScalePosition;

void main() {
    gl_TexCoord[0] = vec4(texture.xy * ScaleTexture.xy + ScaleTexture.zw, 0, 0);
	gl_Position    = vec4(position.xy * ScalePosition.xy + ScalePosition.zw, 0, 1);
}
