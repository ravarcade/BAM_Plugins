#version 120

uniform vec4 Local0;

void main(void)
{
	gl_TexCoord[0] = gl_MultiTexCoord0.xyxy * vec4( 1.0, 1.0, 1.0 / Local0.z, 1.0 / Local0.w );
	gl_Position = gl_Vertex;
}