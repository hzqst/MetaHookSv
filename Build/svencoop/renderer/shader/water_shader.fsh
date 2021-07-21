uniform vec4 watercolor;
uniform vec4 eyepos;
uniform float time;
uniform float fresnelfactor;
uniform float normfactor;

#ifdef REFRACT_ENABLED
uniform sampler2D refractmap;
#endif

uniform sampler2D normalmap;

#ifndef UNDERWATER_ENABLED
uniform sampler2D reflectmap;
#endif

#ifdef DEPTH_ENABLED
uniform vec2 depthfactor;
uniform sampler2D depthrefrmap;
#endif

varying vec4 projpos;
varying vec4 worldpos;

#ifdef DEPTH_ENABLED
varying mat4 viewprojmatrix_inv;

vec3 decodeWorldSpacePosition(vec2 texCoord){
	vec4 clipSpaceLocation;	
	clipSpaceLocation.xy = texCoord * 2.0-1.0;
	clipSpaceLocation.z  = texture2D(depthrefrmap, texCoord).x * 2.0-1.0;
	clipSpaceLocation.w  = 1.0;
	vec4 homogenousLocation = viewprojmatrix_inv * clipSpaceLocation;
	return homogenousLocation.xyz / homogenousLocation.w;
}
#endif

void main()
{
	float flWaterColorAlpha = clamp(watercolor.a, 0.01, 0.99);
	vec4 vWaterColor = vec4(watercolor.x, watercolor.y, watercolor.z, 1.0);

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
	vNormal = (vNormal * 0.25) * 2.0 - 1.0;

	//calculate texcoord
	vec2 vBaseTexCoord = projpos.xy / projpos.w * 0.5 + 0.5;

	vec3 vEyeVect = eyepos.xyz - worldpos.xyz;
	float dist = length(vEyeVect);
	float sinX = abs(vEyeVect.z) / (dist + 0.001);
	float flFresnel = asin(sinX) / (0.5 * 3.14159);
	
	float flOffsetFactor = clamp(flFresnel, 0.0, 1.0) * normfactor;

	flFresnel = 1.0 - flFresnel;

	vec2 vOffsetTexCoord = normalize(vNormal.xyz).xy * flOffsetFactor;

#ifdef REFRACT_ENABLED

	vec2 vRefractTexCoord = vBaseTexCoord + vOffsetTexCoord;
	vec4 vRefractColor = texture2D(refractmap, vRefractTexCoord);
	vRefractColor.a = 1.0;

	vRefractColor = mix(vRefractColor, vWaterColor, flWaterColorAlpha);

#else

	vec4 vRefractColor = vWaterColor;

#endif

#ifdef UNDERWATER_ENABLED

	vec4 vFinalColor = vRefractColor;

	#ifdef GBUFFER_ENABLED
		vFinalColor.a = 1.0;
	    gl_FragData[0] = vFinalColor;
		gl_FragData[1] = vec4(1.0, 1.0, 1.0, 1.0);
		gl_FragData[2] = worldpos;
		gl_FragData[3] = -vNormal;
	#else
		gl_FragColor = vFinalColor;
	#endif

#else

	#ifdef DEPTH_ENABLED
		vec3 vSceneWorld = decodeWorldSpacePosition(vBaseTexCoord);
		float flDiffZ = worldpos.z - vSceneWorld.z;
		float flWaterBlendAlpha = clamp( clamp( depthfactor.x * flDiffZ, 0.0, 1.0 ) + depthfactor.y, 0.0, 1.0 );
	#else
		float flWaterBlendAlpha = 1.0;
	#endif

		//Sample the reflect color (texcoord inverted)
		vec2 vBaseTexCoord2 = vec2(projpos.x, -projpos.y) / projpos.w * 0.5 + 0.5;

		vec2 vReflectTexCoord = vBaseTexCoord2 + vOffsetTexCoord;
		vec4 vReflectColor = texture2D(reflectmap, vReflectTexCoord);
		vReflectColor.a = 1.0;

		if(vReflectColor.x == watercolor.x && vReflectColor.y == watercolor.y && vReflectColor.z == watercolor.z)
		{
			vReflectColor = watercolor;
		}

		float flRefractFactor = clamp(flFresnel * fresnelfactor, 0.0, 1.0);

		vec4 vFinalColor = vRefractColor + vReflectColor * flRefractFactor;

		vFinalColor.a = flWaterBlendAlpha;

		//todo, blend with fog
		float flFogFactor = 0.0;//CalcPixelFogFactor( PIXELFOGTYPE, g_PixelFogParams, g_EyePos, i.worldPos, i.vProjPos.z );

	#ifdef GBUFFER_ENABLED
		vFinalColor.a = 1.0;
	    gl_FragData[0] = vFinalColor;
		gl_FragData[1] = vec4(1.0, 1.0, 1.0, 1.0);
		gl_FragData[2] = worldpos;
		gl_FragData[3] = vNormal;
		gl_FragData[4] = vec4(0.0, 0.0, 0.0, 1.0);
	#else
		gl_FragColor = vFinalColor;
	#endif

#endif
}