#version 130

#extension GL_EXT_texture_array : enable

#define GBUFFER_INDEX_DIFFUSE		0
#define GBUFFER_INDEX_LIGHTMAP		1
#define GBUFFER_INDEX_WORLDNORM		2
#define GBUFFER_INDEX_SPECULAR		3
#define GBUFFER_INDEX_ADDITIVE		4

uniform sampler2D diffuseTex;

/* legacy shade */

uniform float v_lambert;
uniform float v_brightness;
uniform float v_lightgamma;
uniform float r_ambientlight;
uniform float r_shadelight;
uniform float r_blend;
uniform float r_g1;
uniform float r_g3;
uniform vec3 r_plightvec;
uniform vec3 r_colormix;

/* celshade */

uniform float r_celshade_midpoint;
uniform float r_celshade_softness;
uniform vec3 r_celshade_shadow_color;
uniform float r_rimlight_power;
uniform float r_rimlight_smooth;
uniform vec3 r_rimlight_color;
uniform float r_rimdark_power;
uniform float r_rimdark_smooth;
uniform vec3 r_rimdark_color;
uniform float r_outline_dark;

varying vec3 worldpos;
varying vec3 normal;
varying vec4 color;

uniform vec3 viewpos;
uniform vec3 viewdir;

#ifdef SHADOW_CASTER_ENABLED
uniform vec3 entityPos;
#endif

#ifdef CLIP_ABOVE_ENABLED
uniform float clipPlane;
#endif

#ifdef CLIP_UNDER_ENABLED
uniform float clipPlane;
#endif


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

#ifdef STUDIO_NF_CELSHADE

vec3 CelShade(vec3 normalWS, vec3 lightdirWS)
{
	vec3 N = normalWS;
	vec3 V = normalize(worldpos - viewpos);
    vec3 L = lightdirWS;

	L.z *= 0.01;
	L = normalize(L);

    float NoL = dot(-N,L);

    // N dot L
    float litOrShadowArea = smoothstep(r_celshade_midpoint - r_celshade_softness, r_celshade_midpoint + r_celshade_softness, NoL);

#ifdef STUDIO_NF_CELSHADE_FACE
	litOrShadowArea = mix(0.5, 1.0, litOrShadowArea);
#endif

    vec3 litOrShadowColor = mix(r_celshade_shadow_color.xyz, vec3(1.0, 1.0, 1.0), litOrShadowArea);

	vec3 rimColor = vec3(0.0);
	vec3 rimDarkColor = vec3(0.0);

#ifndef STUDIO_NF_CELSHADE_FACE
	//Rim light
	float lambertD = max(0, -NoL);
    float lambertF = max(0, NoL);
    float rim = 1.0 - clamp(dot(V, -N), 0.0, 1.0);

	float rimDot = pow(rim, r_rimlight_power);
	rimDot = lambertF * rimDot;
	float rimIntensity = smoothstep(0, r_rimlight_smooth, rimDot);
	rimColor = pow(rimIntensity, 5.0) * r_rimlight_color;

	rimDot = pow(rim, r_rimdark_power);
    rimDot = lambertD * rimDot;
	rimIntensity = smoothstep(0, r_rimdark_smooth, rimDot);
    rimDarkColor = pow(rimIntensity, 5.0) * r_rimdark_color;
#endif

	return color.xyz * litOrShadowColor + rimColor + rimDarkColor;
}

#endif

void main(void)
{
	vec4 diffuseColor = texture2D(diffuseTex, gl_TexCoord[0].xy);

	vec3 vNormal = normalize(normal.xyz);

	vec4 lightmapColor = color;

#ifdef STUDIO_NF_CELSHADE
	lightmapColor.xyz = CelShade(vNormal, r_plightvec);
#endif

#ifdef OUTLINE_ENABLED
	diffuseColor *= r_outline_dark;
#endif

#ifdef STUDIO_NF_MASKED
    if(diffuseColor.a < 0.5)
        discard;
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

	gl_FragColor = vec4(entityPos.x, entityPos.y, entityPos.z, gl_FragCoord.z);

#else

	#ifdef GBUFFER_ENABLED

		#ifdef TRANSPARENT_ENABLED

			#ifdef STUDIO_NF_ADDITIVE

				gl_FragColor = diffuseColor * lightmapColor;

			#else

				gl_FragData[0] = diffuseColor;
				gl_FragData[1] = lightmapColor;

			#endif

		#else

			vec2 vOctNormal = UnitVectorToOctahedron(vNormal);

			float flDistanceToFragment = distance(worldpos.xyz, viewpos.xyz);

			gl_FragData[GBUFFER_INDEX_DIFFUSE] = diffuseColor;
			gl_FragData[GBUFFER_INDEX_LIGHTMAP] = lightmapColor;
			gl_FragData[GBUFFER_INDEX_WORLDNORM] = vec4(vOctNormal.x, vOctNormal.y, flDistanceToFragment, 0.0);
			gl_FragData[GBUFFER_INDEX_SPECULAR] = vec4(0.0, 0.0, 0.0, 0.0);
			gl_FragData[GBUFFER_INDEX_ADDITIVE] = vec4(0.0, 0.0, 0.0, 0.0);

		#endif

	#else

		gl_FragColor = diffuseColor * lightmapColor;

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