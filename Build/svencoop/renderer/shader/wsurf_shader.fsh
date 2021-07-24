#version 130

#extension GL_EXT_texture_array : enable

#ifdef DIFFUSE_ENABLED
uniform sampler2D diffuseTex;
#endif

#ifdef LIGHTMAP_ENABLED
uniform sampler2DArray lightmapTexArray;
#endif

#ifdef SHADOWMAP_ENABLED
uniform sampler2DArray shadowmapTexArray;
uniform vec3 shadowControl;
varying vec4 shadowcoord[3];
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

#ifdef SHADOWMAP_ENABLED

float shadowCompareDepth(vec4 coord, vec2 off, float layer)
{
	vec4 newcoord = coord + vec4(off.x * SHADOW_TEXTURE_OFFSET, off.y * SHADOW_TEXTURE_OFFSET, 0.0, 0.0);

	float depth0 = texture2DArray(shadowmapTexArray, vec3(newcoord.xy / newcoord.w, layer) ).a;

	float depth1 = newcoord.z / newcoord.w;

	return depth0 < depth1 ? 0.0 : 1.0;
}

vec3 shadowGetPosition(vec4 coord, float layer)
{
	return texture2DArray(shadowmapTexArray, vec3(coord.xy / coord.w, layer) ).xyz;
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

#ifdef SHADOWMAP_ENABLED

	float shadow_high = 1.0;

	#ifdef SHADOWMAP_HIGH_ENABLED
		shadow_high = 0.0;
		shadow_high += shadowCompareDepth(shadowcoord[0], vec2(0.0,0.0), 0.0) * 0.25;
		shadow_high += shadowCompareDepth(shadowcoord[0], vec2( -1.0, -1.0), 0.0) * 0.0625;
		shadow_high += shadowCompareDepth(shadowcoord[0], vec2( -1.0, 0.0), 0.0) * 0.125;
		shadow_high += shadowCompareDepth(shadowcoord[0], vec2( -1.0, 1.0), 0.0) * 0.0625;
		shadow_high += shadowCompareDepth(shadowcoord[0], vec2( 0.0, -1.0), 0.0) * 0.125;
		shadow_high += shadowCompareDepth(shadowcoord[0], vec2( 0.0, 1.0), 0.0) * 0.125;
		shadow_high += shadowCompareDepth(shadowcoord[0], vec2( 1.0, -1.0), 0.0) * 0.0625;
		shadow_high += shadowCompareDepth(shadowcoord[0], vec2( 1.0, 0.0), 0.0) * 0.125;
		shadow_high += shadowCompareDepth(shadowcoord[0], vec2( 1.0, 1.0), 0.0) * 0.0625;
	#endif

	float shadow_medium = 1.0;

	#ifdef SHADOWMAP_MEDIUM_ENABLED
		shadow_medium = 0.0;
		shadow_medium += shadowCompareDepth(shadowcoord[1], vec2(0.0,0.0), 1.0);
		shadow_medium += shadowCompareDepth(shadowcoord[1], vec2(0.035,0.0), 1.0);
		shadow_medium += shadowCompareDepth(shadowcoord[1], vec2(-0.035,0.0), 1.0);
		shadow_medium += shadowCompareDepth(shadowcoord[1], vec2(0.0,0.035), 1.0);
		shadow_medium += shadowCompareDepth(shadowcoord[1], vec2(0.0,-0.035), 1.0);
		shadow_medium *= 0.2;
	#endif

	float shadow_low = 1.0;

	#ifdef SHADOWMAP_LOW_ENABLED
		shadow_low = 0.0;
		shadow_low += shadowCompareDepth(shadowcoord[2], vec2(0.0,0.0), 2.0);
		shadow_low += shadowCompareDepth(shadowcoord[2], vec2(0.035,0.0), 2.0);
		shadow_low += shadowCompareDepth(shadowcoord[2], vec2(-0.035,0.0), 2.0);
		shadow_low += shadowCompareDepth(shadowcoord[2], vec2(0.0,0.035), 2.0);
		shadow_low += shadowCompareDepth(shadowcoord[2], vec2(0.0,-0.035), 2.0);
		shadow_low *= 0.2;
	#endif

	if(false)
	{
		//nothing here
	}
	
	#ifdef SHADOWMAP_HIGH_ENABLED
		else if(shadow_high < 0.95)
		{
			float shadow_alpha = shadowControl.x;

			vec3 scene = worldpos.xyz;
			vec3 caster = shadowGetPosition(shadowcoord[0], 0.0);

			float dist = distance(caster, scene);
			float fade_val = (dist - shadowControl.y) / shadowControl.z;
			shadow_alpha *= 1.0 - clamp(fade_val, 0.0, 1.0);

			shadow_high = 1.0 - shadow_high;
			shadow_medium = 1.0 - shadow_medium;
			shadow_low = 1.0 - shadow_low;

			float shadow_final = shadow_high + shadow_medium + shadow_low;
			shadow_final = clamp(shadow_final, 0.0, 1.0) * shadow_alpha;

			lightmapColor.xyz *= (1.0 - shadow_final);
			
		}
	#endif

	#ifdef SHADOWMAP_MEDIUM_ENABLED
		else if(shadow_medium < 0.95)
		{
			float shadow_alpha = shadowControl.x;

			vec3 scene = worldpos.xyz;
			vec3 caster = shadowGetPosition(shadowcoord[1], 1.0);

			float dist = distance(caster, scene);
			float fade_val = (dist - shadowControl.y) / shadowControl.z;
			shadow_alpha *= 1.0 - clamp(fade_val, 0.0, 1.0);

			shadow_high = 1.0 - shadow_high;
			shadow_medium = 1.0 - shadow_medium;
			shadow_low = 1.0 - shadow_low;

			float shadow_final = shadow_high + shadow_medium + shadow_low;
			shadow_final = clamp(shadow_final, 0.0, 1.0) * shadow_alpha;

			lightmapColor.xyz *= (1.0 - shadow_final);
		}
	#endif

	#ifdef SHADOWMAP_LOW_ENABLED
		else if(shadow_low < 0.95)
		{
			float shadow_alpha = shadowControl.x;

			vec3 scene = worldpos.xyz;
			vec3 caster = shadowGetPosition(shadowcoord[2], 2.0);

			float dist = distance(caster, scene);
			float fade_val = (dist - shadowControl.y) / shadowControl.z;
			shadow_alpha *= 1.0 - clamp(fade_val, 0.0, 1.0);

			shadow_high = 1.0 - shadow_high;
			shadow_medium = 1.0 - shadow_medium;
			shadow_low = 1.0 - shadow_low;

			float shadow_final = shadow_high + shadow_medium + shadow_low;
			shadow_final = clamp(shadow_final, 0.0, 1.0) * shadow_alpha;

			lightmapColor.xyz *= (1.0 - shadow_final);
		}
	#endif

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

#ifdef SHADOW_CASTER_ENABLED

	gl_FragColor.xyz = worldpos.xyz;
	gl_FragColor.w = gl_FragCoord.z;

#else

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

		#ifdef LINEAR_FOG_ENABLED

			float z = gl_FragCoord.z / gl_FragCoord.w;
			float fogFactor = ( gl_Fog.end - z ) / ( gl_Fog.end - gl_Fog.start );
			fogFactor = clamp(fogFactor, 0.0, 1.0);

			vec3 finalColor = gl_FragColor.xyz;

			gl_FragColor.xyz = mix(gl_Fog.color.xyz, finalColor, fogFactor );
			
		#endif

	#endif

#endif
}