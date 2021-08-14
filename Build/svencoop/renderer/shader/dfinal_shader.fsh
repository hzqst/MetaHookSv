#extension GL_EXT_texture_array : enable

#define GBUFFER_INDEX_DIFFUSE		0.0
#define GBUFFER_INDEX_LIGHTMAP		1.0
#define GBUFFER_INDEX_WORLDNORM		2.0
#define GBUFFER_INDEX_SPECULAR		3.0
#define GBUFFER_INDEX_ADDITIVE		4.0

uniform sampler2DArray gbufferTex;
uniform sampler2D depthTex;
uniform sampler2D linearDepthTex;

uniform vec3 viewpos;
uniform mat4 viewmatrix;
uniform mat4 projmatrix;
uniform mat4 invprojmatrix;

uniform float ssrRayStep;
uniform int ssrIterCount;
uniform float ssrDistanceBias;
uniform vec2 ssrFade;

vec2 UnitVectorToHemiOctahedron(vec3 dir) {

	dir.y = max(dir.y, 0.0001);
	dir.xz /= dot(abs(dir), vec3(1.0));

	return clamp(0.5 * vec2(dir.x + dir.z, dir.x - dir.z) + 0.5, 0.0, 1.0);

}

vec3 HemiOctahedronToUnitVector(vec2 coord) {

	coord = 2.0 * coord - 1.0;
	coord = 0.5 * vec2(coord.x + coord.y, coord.x - coord.y);

	float y = 1.0 - dot(vec2(1.0), abs(coord));
	return normalize(vec3(coord.x, y + 0.0001, coord.y));

}

vec2 UnitVectorToOctahedron(vec3 dir) {

    dir.xz /= dot(abs(dir), vec3(1.0));

	// Lower hemisphere
	if (dir.y < 0.0) {
		vec2 orig = dir.xz;
		dir.x = (orig.x >= 0.0 ? 1.0 : -1.0) * (1.0 - abs(orig.y));
        dir.z = (orig.y >= 0.0 ? 1.0 : -1.0) * (1.0 - abs(orig.x));
	}

	return clamp(0.5 * vec2(dir.x, dir.z) + 0.5, 0.0, 1.0);

}

vec3 OctahedronToUnitVector(vec2 coord) {

	coord = 2.0 * coord - 1.0;
	float y = 1.0 - dot(abs(coord), vec2(1.0));

	// Lower hemisphere
	if (y < 0.0) {
		vec2 orig = coord;
		coord.x = (orig.x >= 0.0 ? 1.0 : -1.0) * (1.0 - abs(orig.y));
		coord.y = (orig.y >= 0.0 ? 1.0 : -1.0) * (1.0 - abs(orig.x));
	}

	return normalize(vec3(coord.x, y + 0.0001, coord.y));

}

float random (vec2 uv) {
	return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453123); //simple random function
}

vec4 GenerateBasicColor(vec2 texcoord)
{
    vec4 diffuseColor = texture2DArray(gbufferTex, vec3(texcoord, GBUFFER_INDEX_DIFFUSE));

    vec4 resultColor = diffuseColor;
    resultColor.a = 1.0;

    return resultColor;
}

vec4 GenerateAdditiveColor(vec2 texcoord)
{
    vec4 additiveColor = texture2DArray(gbufferTex, vec3(texcoord, GBUFFER_INDEX_ADDITIVE));

    vec4 resultColor = additiveColor;
    resultColor.a = 1.0;

    return resultColor;
}

vec3 GenerateViewPositionFromDepth(vec2 texcoord, float depth) {
    vec2 texcoord2 = vec2((texcoord.x - 0.5) * 2.0, (texcoord.y - 0.5) * 2.0);
	vec4 ndc = vec4(texcoord2.xy, depth, 1.0);
	vec4 inversed = invprojmatrix * ndc;// going back from projected
	inversed /= inversed.w;
	return inversed.xyz;
}

vec2 GenerateProjectedPosition(vec3 pos){
	vec4 samplePosition = projmatrix * vec4(pos, 1.0);
	samplePosition.xy = (samplePosition.xy / samplePosition.w) * 0.5 + 0.5;
	return samplePosition.xy;
}

vec3 GenerateWorldNormal(vec2 texcoord)
{
    vec4 worldnormColor = texture2DArray(gbufferTex, vec3(texcoord, GBUFFER_INDEX_WORLDNORM));
    vec3 normalworld = OctahedronToUnitVector(worldnormColor.xy);

    return normalworld;
}

vec3 GenerateViewNormal(vec2 texcoord)
{
    return normalize((viewmatrix * vec4(GenerateWorldNormal(texcoord), 0.0) ).xyz);
}

vec4 VignetteColor(vec4 c, vec2 win_bias)
{
    // convert window coord to [-1, 1] range
    vec2 wpos = 2.0*(win_bias - vec2(0.5, 0.5)); 

    // calculate distance from origin
    float r = length(wpos);
    r = 1.0 - smoothstep(ssrFade.x, ssrFade.y, r);
	
    c.a *= r;

    return c;
}

vec4 ScreenSpaceReflectionInternal(vec3 position, vec3 reflection)
{
	vec3 step = ssrRayStep * reflection;
	vec3 marchingPosition = position + step;
	float delta;
	float depthFromScreen;
	vec2 screenPosition;

    int i = 0;
	for (; i < ssrIterCount; i++) {
		screenPosition = GenerateProjectedPosition(marchingPosition);
		depthFromScreen = abs(GenerateViewPositionFromDepth(screenPosition, texture2D(depthTex, screenPosition).x).z);
		delta = abs(marchingPosition.z) - depthFromScreen;
		if (abs(delta) < ssrDistanceBias) {
            if(screenPosition.x < 0.0 || screenPosition.x > 1.0 || screenPosition.y < 0.0 || screenPosition.y > 1.0){
                return vec4(0.0);
            }

			return VignetteColor(GenerateBasicColor(screenPosition), screenPosition);
		}
        #ifdef SSR_BINARY_SEARCH_ENABLED
		if (delta > 0.0) {
			break;
		}
        #endif
		#ifdef SSR_ADAPTIVE_STEP_ENABLED
			float directionSign = sign(abs(marchingPosition.z) - depthFromScreen);
			//this is sort of adapting step, should prevent lining reflection by doing sort of iterative converging
			//some implementation doing it by binary search, but I found this idea more cheaty and way easier to implement
			step = step * (1.0 - ssrRayStep * max(directionSign, 0.0));
			marchingPosition += step * (-directionSign);
		#else
			marchingPosition += step;
		#endif
		#ifdef SSR_EXPONENTIAL_STEP_ENABLED
			step *= 1.05;
		#endif
    }
	#ifdef SSR_BINARY_SEARCH_ENABLED
		for(; i < ssrIterCount; i++){
			
			step *= 0.5;
			marchingPosition = marchingPosition - step * sign(delta);
			
			screenPosition = GenerateProjectedPosition(marchingPosition);
			depthFromScreen = abs(GenerateViewPositionFromDepth(screenPosition, texture2D(depthTex, screenPosition).x).z);
			delta = abs(marchingPosition.z) - depthFromScreen;
			
			if (abs(delta) < ssrDistanceBias) {
                if(screenPosition.x < 0.0 || screenPosition.x > 1.0 || screenPosition.y < 0.0 || screenPosition.y > 1.0){
                    return vec4(0.0);
                }

				return VignetteColor(GenerateBasicColor(screenPosition), screenPosition);
			}
		}
	#endif
	
    return vec4(0.0);
}

vec4 ScreenSpaceReflection()
{
    vec3 position = GenerateViewPositionFromDepth(gl_TexCoord[0].xy, texture2D(depthTex, gl_TexCoord[0].xy).x);
    vec3 viewnormal = GenerateViewNormal(gl_TexCoord[0].xy);

    vec3 reflectionDirection = normalize(reflect(position, viewnormal));

    return ScreenSpaceReflectionInternal(position, reflectionDirection);
}

vec4 GenerateFinalColor(vec2 texcoord)
{
    vec4 diffuseColor = texture2DArray(gbufferTex, vec3(texcoord, GBUFFER_INDEX_DIFFUSE));
    vec4 lightmapColor = texture2DArray(gbufferTex, vec3(texcoord, GBUFFER_INDEX_LIGHTMAP));

#ifdef SSR_ENABLED
    vec4 specularColor = texture2DArray(gbufferTex, vec3(texcoord, GBUFFER_INDEX_SPECULAR));
    if(specularColor.g > 0.0)
    {
        vec4 ssr = ScreenSpaceReflection();

        diffuseColor.xyz = mix(diffuseColor.xyz, ssr.xyz, specularColor.g * ssr.a);
    }
#endif

    vec4 resultColor = diffuseColor * lightmapColor + GenerateAdditiveColor(texcoord);

    resultColor.a = 1.0;

    return resultColor;
}

void main()
{
    vec4 finalColor = GenerateFinalColor(gl_TexCoord[0].xy);

#ifdef LINEAR_FOG_ENABLED

    vec4 linearDepthColor = texture2D(linearDepthTex, gl_TexCoord[0].xy);

	float z = linearDepthColor.x;
	float fogFactor = ( gl_Fog.end - z ) / ( gl_Fog.end - gl_Fog.start );
	fogFactor = clamp(fogFactor, 0.0, 1.0);

	gl_FragColor.xyz = mix(gl_Fog.color.xyz, finalColor.xyz, fogFactor );

#else

    gl_FragColor = finalColor;

#endif

}