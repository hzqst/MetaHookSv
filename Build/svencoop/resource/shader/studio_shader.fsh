#version 130
uniform sampler2D diffuseTex;

varying vec4 worldpos;
varying vec4 normal;
varying vec4 color;

void main(void)
{
	vec4 diffuseColor = texture2D(diffuseTex, gl_TexCoord[0].xy);

#ifdef GBUFFER_ENABLED

	#ifdef TRANSPARENT_ENABLED

		#ifdef STUDIO_NF_ADDITIVE

			gl_FragColor = diffuseColor * color;

		#else

			gl_FragData[0] = diffuseColor;
			gl_FragData[1] = color;

		#endif

	#else

		gl_FragData[0] = diffuseColor;
		gl_FragData[1] = color;
		gl_FragData[2] = worldpos;
		gl_FragData[3] = normal;
		gl_FragData[4] = vec4(0.0, 0.0, 0.0, 1.0);

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
}