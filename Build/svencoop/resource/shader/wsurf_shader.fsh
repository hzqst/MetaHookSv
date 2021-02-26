#version 130

#ifdef DIFFUSE_ENABLED
uniform sampler2D diffuseTex;
#endif

#ifdef LIGHTMAP_ENABLED
uniform sampler2DArray lightmapTexArray;
#endif

#ifdef DETAILTEXTURE_ENABLED
uniform sampler2D detailTex;
#endif

void main()
{
	#ifdef DIFFUSE_ENABLED
	vec4 diffuseColor = texture2D(diffuseTex, gl_TexCoord[0].xy);
	#else
	vec4 diffuseColor = gl_Color;
	#endif

	#ifdef LIGHTMAP_ENABLED
	vec4 lightmapColor = texture2DArray(lightmapTexArray, gl_TexCoord[1].xyz);
	#else
	vec4 lightmapColor = vec4(1.0, 1.0, 1.0, 1.0);
	#endif

	#ifdef DETAILTEXTURE_ENABLED
	vec4 detailColor = texture2D(detailTex, gl_TexCoord[2].xy);
	#else
	vec4 detailColor = vec4(1.0, 1.0, 1.0, 1.0);
	#endif

	gl_FragColor = diffuseColor * lightmapColor * detailColor;
}