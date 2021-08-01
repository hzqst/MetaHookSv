#extension GL_EXT_texture_array : enable

#define GBUFFER_INDEX_DIFFUSE		0.0
#define GBUFFER_INDEX_LIGHTMAP		1.0
#define GBUFFER_INDEX_WORLD			2.0
#define GBUFFER_INDEX_NORMAL		3.0
#define GBUFFER_INDEX_SPECULAR		4.0
#define GBUFFER_INDEX_ADDITIVE		5.0

uniform sampler2DArray gbufferTex;
uniform sampler2D depthTex;
uniform vec3 clipInfo;

float reconstructCSZ(float d) {
    return (clipInfo[0] / (clipInfo[1] * d + clipInfo[2]));
}

void main()
{
    vec4 diffuseColor = texture2DArray(gbufferTex, vec3(gl_TexCoord[0].xy, GBUFFER_INDEX_DIFFUSE));
    vec4 lightmapColor = texture2DArray(gbufferTex, vec3(gl_TexCoord[0].xy, GBUFFER_INDEX_LIGHTMAP));
    vec4 additiveColor = texture2DArray(gbufferTex, vec3(gl_TexCoord[0].xy, GBUFFER_INDEX_ADDITIVE));

    vec4 finalColor = diffuseColor * lightmapColor + additiveColor;

#ifdef LINEAR_FOG_ENABLED

    vec4 depthColor = texture2D(depthTex, gl_TexCoord[0].xy);

	float z = reconstructCSZ(depthColor.x);
	float fogFactor = ( gl_Fog.end - z ) / ( gl_Fog.end - gl_Fog.start );
	fogFactor = clamp(fogFactor, 0.0, 1.0);

	gl_FragColor.xyz = mix(gl_Fog.color.xyz, finalColor.xyz, fogFactor );

#else

    gl_FragColor = finalColor;

#endif

}