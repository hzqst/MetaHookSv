#ifdef TEXARRAY_ENABLED

uniform sampler2DArray basetex;
uniform float layer;

#endif

void main() 
{ 
	#ifdef TEXARRAY_ENABLED
	
	gl_FragColor = texture2DArray(basetex, vec3(gl_TexCoord[0].xy, layer) );

	#endif
}