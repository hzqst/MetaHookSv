#ifdef DECAL_PASS

varying vec4 projpos;
uniform mat4 decalToWorldMatrix;
uniform mat4 worldToDecalMatrix;
uniform float decalScale;

#endif

void main(void)
{
#ifdef DECAL_PASS

	vec4 world = decalToWorldMatrix * vec4(gl_Vertex.xyz * decalScale, gl_Vertex.w);
	gl_Position = gl_ModelViewProjectionMatrix * world;
	projpos = gl_Position;
	
#else

	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();

#endif
}