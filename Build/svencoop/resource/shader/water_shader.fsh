uniform vec4 waterfogcolor;
uniform vec4 eyepos;
uniform float time;
uniform float fresnel;
uniform float normfactor;

#ifdef DEPTH_ENABLED
uniform vec2 clipinfo;
uniform float depthfactor;
#endif

uniform sampler2D normalmap;
uniform sampler2D refractmap;

#ifndef UNDERWATER_ENABLED
uniform sampler2D reflectmap;
#endif

#ifdef DEPTH_ENABLED
uniform sampler2D depthrefrmap;
#endif

varying vec4 projpos;
varying vec4 worldpos;

float linearizeDepth(float nearz, float farz, float depth)
{
	depth = 2.0 * depth - 1.0;
	return (2.0 * nearz) / (farz + nearz - depth * (farz - nearz));
}

void main()
{
	//calculate the normal texcoord and sample the normal vector from texture
	vec2 vNormTexCoord1 = vec2(0.2, 0.15) * time + gl_TexCoord[0].xy; 
	vec2 vNormTexCoord2 = vec2(-0.13, 0.11) * time + gl_TexCoord[0].xy;
	vec2 vNormTexCoord3 = vec2(-0.14, -0.16) * time + gl_TexCoord[0].xy;
	vec2 vNormTexCoord4 = vec2(0.17, 0.15) * time + gl_TexCoord[0].xy;
	vec4 vNorm1 = texture2D(normalmap, vNormTexCoord1);
	vec4 vNorm2 = texture2D(normalmap, vNormTexCoord2);
	vec4 vNorm3 = texture2D(normalmap, vNormTexCoord3);
	vec4 vNorm4 = texture2D(normalmap, vNormTexCoord4);
	vec4 vNormal = vNorm1 + vNorm2 + vNorm3 + vNorm4;
	vNormal = (vNormal / 4.0) * 2.0 - 1.0;

	//calculate texcoord
	vec2 vBaseTexCoord = projpos.xy / projpos.w * 0.5 + 0.5;

	//fresnel factor
	vec3 vEyeVect = eyepos.xyz - worldpos.xyz;
	float dist = length(vEyeVect);
	float sinX = abs(vEyeVect.z) / (dist + 0.001);
	float fresnelX = asin(sinX) / (0.5 * 3.14159);

	float flOffsetFactor = clamp(fresnelX, 0.001, 1.0) * normfactor;

	vec2 vOffsetTexCoord = normalize(vNormal.xyz).xy * flOffsetFactor;

	//sample the refract color

	vec2 vRefractTexCoord = vBaseTexCoord + vOffsetTexCoord;
	vec4 vRefractColor = texture2D(refractmap, vRefractTexCoord);
	vRefractColor.a = 1.0;

	if(vRefractColor.x == waterfogcolor.x && vRefractColor.y == waterfogcolor.y && vRefractColor.z == waterfogcolor.z )
		discard;

	

#ifdef UNDERWATER_ENABLED

		//lerp waterfog color and refraction color
		float flWaterColorAlpha = clamp(waterfogcolor.a, 0.01, 0.9);
		vec4 vWaterColor = vec4(waterfogcolor.x, waterfogcolor.y, waterfogcolor.z, 1.0);

		vec4 vFinalColor2 = vRefractColor * (1.0 - flWaterColorAlpha) + vWaterColor * flWaterColorAlpha;

	#ifdef GBUFFER_ENABLED
		vFinalColor2.a = 1.0;
	    gl_FragData[0] = vFinalColor2;
		gl_FragData[1] = vec4(1.0, 1.0, 1.0, 1.0);
		gl_FragData[2] = worldpos;
		gl_FragData[3] = -vNormal;
	#else
		gl_FragColor = vFinalColor2;
	#endif

#else

	#ifdef DEPTH_ENABLED
		vec4 vDepthColor = texture2D(depthrefrmap, vBaseTexCoord);
		float flDepthFactor = pow(vDepthColor.x, depthfactor);
	#endif

		//sample the reflect color(texcoord inverted)
		vec2 vBaseTexCoord2 = vec2(projpos.x, -projpos.y) / projpos.w * 0.5 + 0.5;

		vec2 vReflectTexCoord = vBaseTexCoord2 + vOffsetTexCoord;
		vec4 vReflectColor = texture2D(reflectmap, vReflectTexCoord);
		vReflectColor.a = 1.0;

		float flRefractFactor = clamp(fresnelX * fresnel, 0.05, 0.999);

		if(vReflectColor.x == waterfogcolor.x && vReflectColor.y == waterfogcolor.y && vReflectColor.z == waterfogcolor.z )
			flRefractFactor = 1.0;

		//lerp the reflection and refraction color by fresnel
		vec4 vFinalColor = vReflectColor * (1.0-flRefractFactor) + vRefractColor * flRefractFactor;

		float flWaterColorAlpha = clamp(waterfogcolor.a, 0.01, 0.9);

		vec4 vWaterColor = vec4(waterfogcolor.x, waterfogcolor.y, waterfogcolor.z, 1.0);

		//lerp waterfog color
		vec4 vFinalColor2 = vFinalColor * (1.0-flWaterColorAlpha) + vWaterColor * flWaterColorAlpha;

	#ifdef DEPTH_ENABLED
		vFinalColor2.a = flDepthFactor;
	#else
		vFinalColor2.a = 1.0;
	#endif

	#ifdef GBUFFER_ENABLED
		vFinalColor2.a = 1.0;
	    gl_FragData[0] = vFinalColor2;
		gl_FragData[1] = vec4(1.0, 1.0, 1.0, 1.0);
		gl_FragData[2] = worldpos;
		gl_FragData[3] = vNormal;
		gl_FragData[4] = vec4(0.0, 0.0, 0.0, 1.0);
	#else
		gl_FragColor = vFinalColor2;
	#endif

#endif
}