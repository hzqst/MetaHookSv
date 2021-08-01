#ifdef VOLUME_ENABLED
varying vec4 projpos;
#endif

void main(void)
{
	gl_TexCoord[0] = gl_MultiTexCoord0;

#ifdef VOLUME_ENABLED
	projpos = gl_Position;
#else
	gl_Position = ftransform();
#endif
	
}