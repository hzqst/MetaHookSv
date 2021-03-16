#ifdef LIGHT_PASS_VOLUME
varying vec4 projpos;
#endif

void main(void)
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
	
#ifdef LIGHT_PASS_VOLUME
	projpos = gl_Position;
#endif
}