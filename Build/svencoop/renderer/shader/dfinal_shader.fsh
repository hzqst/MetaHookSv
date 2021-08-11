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

uniform float rayStep = 3.0;
uniform int iterationCount = 100;
uniform float distanceBias = 0.1;
uniform int sampleCount = 4;
uniform bool isSamplingEnabled = false;
uniform bool isExponentialStepEnabled = true;
uniform bool isAdaptiveStepEnabled = true;
uniform bool isBinarySearchEnabled = true;

varying vec3 fragpos;

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

vec4 GenerateFinalColor(vec2 texcoord)
{
    vec4 diffuseColor = texture2DArray(gbufferTex, vec3(texcoord, GBUFFER_INDEX_DIFFUSE));
    vec4 lightmapColor = texture2DArray(gbufferTex, vec3(texcoord, GBUFFER_INDEX_LIGHTMAP));
    vec4 additiveColor = texture2DArray(gbufferTex, vec3(texcoord, GBUFFER_INDEX_ADDITIVE));

    return diffuseColor * lightmapColor + additiveColor;
}

vec3 GeneratePositionFromDepth(vec2 texcoord, float depth) {
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

vec4 ScreenSpaceReflectionInternal(vec3 position, vec3 reflection)
{
	vec3 step = rayStep * reflection;
	vec3 marchingPosition = position + step;
	float delta;
	float depthFromScreen;
	vec2 screenPosition;

    int i = 0;
	for (; i < iterationCount; i++) {
		screenPosition = GenerateProjectedPosition(marchingPosition);
		depthFromScreen = abs(GeneratePositionFromDepth(screenPosition, texture2D(depthTex, screenPosition).x).z);
		delta = abs(marchingPosition.z) - depthFromScreen;
		if (abs(delta) < distanceBias) {
			return GenerateFinalColor(screenPosition);
		}
		if (isBinarySearchEnabled && delta > 0.0) {
			break;
		}
		if (isAdaptiveStepEnabled){
			float directionSign = sign(abs(marchingPosition.z) - depthFromScreen);
			//this is sort of adapting step, should prevent lining reflection by doing sort of iterative converging
			//some implementation doing it by binary search, but I found this idea more cheaty and way easier to implement
			step = step * (1.0 - rayStep * max(directionSign, 0.0));
			marchingPosition += step * (-directionSign);
		}
		else {
			marchingPosition += step;
		}
		if (isExponentialStepEnabled){
			step *= 1.05;
		}
    }
	if(isBinarySearchEnabled){
		for(; i < iterationCount; i++){
			
			step *= 0.5;
			marchingPosition = marchingPosition - step * sign(delta);
			
			screenPosition = GenerateProjectedPosition(marchingPosition);
			depthFromScreen = abs(GeneratePositionFromDepth(screenPosition, texture2D(depthTex, screenPosition).x).z);
			delta = abs(marchingPosition.z) - depthFromScreen;
			
			if (abs(delta) < distanceBias) {
				return GenerateFinalColor(screenPosition);
			}
		}
	}
	
    return vec4(0.0);
}

vec4 ScreenSpaceReflection()
{
    vec3 position = GeneratePositionFromDepth(gl_TexCoord[0].xy, texture2D(depthTex, gl_TexCoord[0].xy).x);
    
    vec4 worldnormColor = texture2DArray(gbufferTex, vec3(gl_TexCoord[0].xy, GBUFFER_INDEX_WORLDNORM));
    vec3 normalworld = OctahedronToUnitVector(worldnormColor.xy);

    vec4 normal = viewmatrix * vec4(normalworld, 0.0);

    vec3 reflectionDirection = normalize(reflect(position, normalize(normal.xyz)));

    return ScreenSpaceReflectionInternal(position, reflectionDirection);
}

void main()
{
    vec4 finalColor = GenerateFinalColor(gl_TexCoord[0].xy);
    vec4 specularColor = texture2DArray(gbufferTex, vec3(gl_TexCoord[0].xy, GBUFFER_INDEX_SPECULAR));

    if(specularColor.g > 0.0)
    {
        vec4 ssr = ScreenSpaceReflection();

        finalColor = mix(finalColor, ssr, 0.5);
    }

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