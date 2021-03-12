uniform sampler2D lumtex;
varying float lum;

void main() 
{
	lum = texture2D(lumtex, vec2(0.5, 0.5)).x;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
}