
varying vec3 fragpos;

void main(void)
{
	fragpos = gl_Color;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
}