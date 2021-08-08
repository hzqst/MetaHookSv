#version 130

#define GBUFFER_INDEX_DIFFUSE		0
#define GBUFFER_INDEX_LIGHTMAP		1
#define GBUFFER_INDEX_WORLDNORM		2
#define GBUFFER_INDEX_SPECULAR		3
#define GBUFFER_INDEX_ADDITIVE		4

uniform sampler2D diffuseTex;

varying vec3 worldpos;
varying vec3 normal;
varying vec4 color;

uniform vec3 viewpos;

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

void main(void)
{
	vec4 diffuseColor = texture2D(diffuseTex, gl_TexCoord[0].xy);

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

				gl_FragColor = diffuseColor * color;

			#else

				gl_FragData[0] = diffuseColor;
				gl_FragData[1] = color;

			#endif

		#else

			vec3 vNormal = normal.xyz;

			vec2 vOctNormal = UnitVectorToOctahedron(vNormal);

			float flDistanceToFragment = distance(worldpos.xyz, viewpos.xyz);

			gl_FragData[GBUFFER_INDEX_DIFFUSE] = diffuseColor;
			gl_FragData[GBUFFER_INDEX_LIGHTMAP] = color;
			gl_FragData[GBUFFER_INDEX_WORLDNORM] = vec4(vOctNormal.x, vOctNormal.y, flDistanceToFragment, 0.0);
			gl_FragData[GBUFFER_INDEX_SPECULAR] = vec4(0.0, 0.0, 0.0, 0.0);
			gl_FragData[GBUFFER_INDEX_ADDITIVE] = vec4(0.0, 0.0, 0.0, 0.0);

		#endif

	#else

		gl_FragColor = diffuseColor * color;

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