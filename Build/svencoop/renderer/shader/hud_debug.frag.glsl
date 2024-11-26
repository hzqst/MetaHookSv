#ifdef TEXARRAY_ENABLED

uniform sampler2DArray basetex;
uniform float layer;

#endif

#ifdef SHADOW_ENABLED

uniform sampler2D basetex;

#endif

void main() 
{ 
	#ifdef TEXARRAY_ENABLED
	
	gl_FragColor = texture2DArray(basetex, vec3(gl_TexCoord[0].xy, layer) );

	#endif

	#ifdef SHADOW_ENABLED
	
	vec4 color = texture2D(basetex, gl_TexCoord[0].xy);

	color.x = pow(color.x, 10.0);
	color.y = color.x;
	color.z = color.x;
	color.a = 1.0;

	gl_FragColor = color;
	#endif
}