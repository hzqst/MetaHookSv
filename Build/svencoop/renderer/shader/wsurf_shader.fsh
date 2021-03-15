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

#ifdef NORMALTEXTURE_ENABLED
uniform sampler2D normalTex;
#endif

#ifdef PARALLAXTEXTURE_ENABLED

uniform sampler2D parallaxTex;
uniform float parallaxScale;

varying vec3 tangentViewPos;
varying vec3 tangentFragPos;

#endif

#ifdef CLIP_ABOVE_ENABLED
uniform float clipPlane;
#endif

#ifdef CLIP_UNDER_ENABLED
uniform float clipPlane;
#endif

varying vec4 worldpos;
varying vec4 normal;
varying vec4 tangent;
//varying vec4 bitangent;
varying vec4 color;

#ifdef NORMALTEXTURE_ENABLED

vec3 NormalMapping()
{
    // Create TBN matrix. tangent to world?
	vec3 bitangent = cross(normalize(normal.xyz), normalize(tangent.xyz));
    mat3 TBN = mat3(normalize(tangent.xyz), normalize(bitangent.xyz), normalize(normal.xyz));

    // Sample tangent space normal vector from normal map and remap it from [0, 1] to [-1, 1] range.
    vec3 n = texture2D(normalTex, vec2(gl_TexCoord[0].x / gl_TexCoord[3].x, gl_TexCoord[0].y / gl_TexCoord[3].y)).xyz;
    n = normalize(n * 2.0 - 1.0);

    // Multiple vector by the TBN matrix to transform the normal from tangent space to world space.
    n = normalize(TBN * n);

    return n;
}

#endif

#ifdef PARALLAXTEXTURE_ENABLED

vec2 ParallaxMapping(vec3 viewDir)
{ 
    const float numLayers = 100;

    float layerDepth = 1.0 / numLayers;

    float currentLayerDepth = 0.0;

    vec2 p = viewDir.xy * parallaxScale;

    vec2 deltaTexCoords = p / numLayers;

	vec2 mainTexCoods = gl_TexCoord[0].xy;

    vec2 currentTexCoords = mainTexCoods;
    float currentDepthMapValue = texture2D(parallaxTex, vec2(currentTexCoords.x / gl_TexCoord[4].x, currentTexCoords.y / gl_TexCoord[4].y) ).r;

    while(currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = texture2D(parallaxTex, vec2(currentTexCoords.x / gl_TexCoord[4].x, currentTexCoords.y / gl_TexCoord[4].y) ).r;
        currentLayerDepth += layerDepth;  
    }

	/*vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
	float afterHeight  = currentDepthMapValue - currentLayerDepth;
	float beforeHeight = currentLayerDepth + layerDepth - texture2D(parallaxTex, vec2(prevTexCoords.x / gl_TexCoord[4].x, prevTexCoords.y / gl_TexCoord[4].y) ).r;

	float weight = afterHeight / (afterHeight + beforeHeight);
	vec2 finalTexCoords = mix(currentTexCoords, prevTexCoords, weight);*/

    return currentTexCoords;   
}

#endif

void main()
{
#ifdef DIFFUSE_ENABLED

	#ifdef PARALLAXTEXTURE_ENABLED
		
		vec3 viewDir = normalize(tangentViewPos - tangentFragPos);

		vec4 diffuseColor = texture2D(diffuseTex, ParallaxMapping(viewDir));

	#else

		vec4 diffuseColor = texture2D(diffuseTex, gl_TexCoord[0].xy);

	#endif

#else

	vec4 diffuseColor = color;

#endif

#ifdef LIGHTMAP_ENABLED

	vec4 lightmapColor = texture2DArray(lightmapTexArray, gl_TexCoord[1].xyz);

#else

	vec4 lightmapColor = vec4(1.0, 1.0, 1.0, 1.0);

#endif

#ifdef DETAILTEXTURE_ENABLED

	vec2 detailTexCoord = vec2(gl_TexCoord[0].x * gl_TexCoord[2].x, gl_TexCoord[0].y * gl_TexCoord[2].y);
	vec4 detailColor = texture2D(detailTex, detailTexCoord);
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

#ifdef GBUFFER_ENABLED

	gl_FragData[0] = diffuseColor * detailColor;
	gl_FragData[1] = lightmapColor;
	gl_FragData[2] = worldpos;
	gl_FragData[3] = normal;
	gl_FragData[4] = vec4(0.0, 0.0, 0.0, 1.0);

	#ifdef NORMALTEXTURE_ENABLED

		gl_FragData[3] = vec4(NormalMapping(), 0.0);

	#endif

#else

	#ifdef TRANSPARENT_ENABLED

		gl_FragColor = diffuseColor * lightmapColor * detailColor * color;

	#else

		gl_FragColor = diffuseColor * lightmapColor * detailColor;

	#endif
#endif

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