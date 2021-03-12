#version 130

#extension GL_EXT_texture_array : enable

#ifdef DIFFUSE_ENABLED
uniform sampler2D diffuseTex;
#endif

#ifdef LIGHTMAP_ENABLED
uniform sampler2DArray lightmapTexArray;
#endif

#ifdef DETAILTEXTURE_ENABLED
uniform sampler2D detailTex;
#endif

#ifdef CLIP_ABOVE_ENABLED
uniform float clipPlane;
#endif

#ifdef CLIP_UNDER_ENABLED
uniform float clipPlane;
#endif

#ifdef FOG_ENABLED

#endif

varying vec4 worldpos;

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
    detailColor.xyz *= 2.0;
    detailColor.a = 1.0;

#else

	vec4 detailColor = vec4(1.0, 1.0, 1.0, 1.0);

#endif

#ifdef CLIP_ABOVE_ENABLED
	if (worldpos.z > clipPlane)
		discard;
#endif

#ifdef CLIP_UNDER_ENABLED
	if (worldpos.z < clipPlane)
		discard;
#endif

	gl_FragColor = diffuseColor * lightmapColor * detailColor;

#ifdef LINEAR_FOG_ENABLED
	float z = gl_FragCoord.z / gl_FragCoord.w;
	float fogFactor = ( gl_Fog.end - z ) / ( gl_Fog.end - gl_Fog.start );
	fogFactor = clamp(fogFactor, 0.0, 1.0);

	vec3 finalColor = gl_FragColor.xyz;

	gl_FragColor.xyz = mix(gl_Fog.color.xyz, finalColor, fogFactor );
#endif

#ifdef EXP2_FOG_ENABLED
	float z = gl_FragCoord.z / gl_FragCoord.w;
	float fogFactor = exp( (-1.0) * (gl_Fog.density * z) * (gl_Fog.density * z) );
	fogFactor = clamp(fogFactor, 0.0, 1.0);

	vec3 finalColor = gl_FragColor.xyz;

	gl_FragColor.xyz = mix(finalColor, gl_Fog.color.xyz, fogFactor );
#endif
}