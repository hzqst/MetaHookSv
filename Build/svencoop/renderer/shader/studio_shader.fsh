#version 130

#define GBUFFER_INDEX_DIFFUSE		0
#define GBUFFER_INDEX_LIGHTMAP		1
#define GBUFFER_INDEX_WORLD			2
#define GBUFFER_INDEX_NORMAL		3
#define GBUFFER_INDEX_SPECULAR		4
#define GBUFFER_INDEX_ADDITIVE		5

uniform sampler2D diffuseTex;

varying vec3 worldpos;
varying vec3 normal;
varying vec4 color;

#ifdef SHADOW_CASTER_ENABLED
uniform vec3 entityPos;
#endif

#ifdef CLIP_ABOVE_ENABLED
uniform float clipPlane;
#endif

#ifdef CLIP_UNDER_ENABLED
uniform float clipPlane;
#endif

vec2 encodeNormal(vec3 n)
{
    vec2 enc = normalize(n.xy) * (sqrt(-n.z*0.5+0.5));
    enc = vec2(enc.x*0.5+0.5, enc.y*0.5+0.5);
    return enc;
}

vec3 decodeNormal(vec2 enc)
{
    vec4 nn = vec4(enc.x * 2.0, enc.y * 2.0, 0.0, 0.0) + vec4(-1.0, -1.0, 1.0, -1.0);
    float l = dot(nn.xyz, -nn.xyw);
    nn.z = l;
    nn.xy *= sqrt(l);
    return vec3(nn.x * 2.0, nn.y * 2.0, nn.z * 2.0) + vec3(0.0, 0.0, -1.0);
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

			//vec2 normalenc = encodeNormal(normal.xyz);

			gl_FragData[GBUFFER_INDEX_DIFFUSE] = diffuseColor;
			gl_FragData[GBUFFER_INDEX_LIGHTMAP] = color;
			gl_FragData[GBUFFER_INDEX_WORLD] = vec4(worldpos.xyz, 0.0);
			gl_FragData[GBUFFER_INDEX_NORMAL] = vec4(normal.xyz, 0.0);
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