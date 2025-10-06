#version 430

#include "common.h"

uniform mat4 u_entityMatrix;

layout(binding = 0) uniform sampler2D portalTex;

#if defined(PORTAL_OVERLAY_TEXTURE_ENABLED)
layout(binding = 1) uniform sampler2D overlayTex;
#endif

in vec4 v_projpos;
in vec3 v_worldpos;
in vec3 v_normal;
in vec2 v_diffusetexcoord;

layout(location = 0) out vec4 out_Diffuse;

void main()
{
	float flDistanceToFragment = distance(v_worldpos.xyz, GetCameraViewPos(GetCameraViewIndex()));

	vec3 vNormal = v_normal;

#if defined(SHADOW_CASTER_ENABLED)

	#if defined(LINEAR_DEPTH_ENABLED)
		float newDepth = flDistanceToFragment / GetCameraZFar(GetCameraViewIndex());

		gl_FragDepth = newDepth;
	#endif

	out_Diffuse = vec4(flDistanceToFragment, 0.0, 0.0, 1.0);

#else

	vec4 vFinalColor = vec4(0.0);

	#if defined(PORTAL_TEXCOORD_ENABLED)

		vec2 vPortalTexCoord = v_projpos.xy / v_projpos.w * 0.5 + 0.5;

	#elif defined(PORTAL_REVERSE_TEXCOORD_ENABLED)

		vec2 vPortalTexCoord = v_projpos.xy / v_projpos.w * 0.5 + 0.5;
		vPortalTexCoord.x = -vPortalTexCoord.x;

	#else

	vec2 vPortalTexCoord = vec2(v_diffusetexcoord.x, -v_diffusetexcoord.y);//v_projpos.xy / v_projpos.w * 0.5 + 0.5;

	#endif

	vec4 vPortalColor = texture(portalTex, vPortalTexCoord);
	
	vPortalColor = ProcessOtherGammaColor(vPortalColor);

	vFinalColor = vPortalColor;

	#if defined(PORTAL_OVERLAY_TEXTURE_ENABLED)

		vec2 vOverlayTexCoord = vec2(v_diffusetexcoord.x, v_diffusetexcoord.y);
		vec4 vOverlayColor = texture(overlayTex, vOverlayTexCoord);

		vOverlayColor = ProcessDiffuseColor(vOverlayColor);

		if (vOverlayColor.a > 0.5)
			vFinalColor = vOverlayColor;

	#endif

	#if defined(GBUFFER_ENABLED)

		vec4 specularColor = vec4(0.0);

		vec2 vOctNormal = UnitVectorToOctahedron(vNormal);

		#if defined(SPECULARTEXTURE_ENABLED)
		
			vec2 specularTexCoord = vec2(baseTexcoord.x * v_speculartexcoord.x, baseTexcoord.y * v_speculartexcoord.y);
			specularColor.xy = texture(specularTex, specularTexCoord).xy;

		#endif

		#if defined(DECAL_ENABLED)

			out_Diffuse = ProcessLinearBlendShift(vFinalColor);
			out_WorldNorm = vec4(vOctNormal.x, vOctNormal.y, flDistanceToFragment, out_Diffuse.a);
			out_Specular = specularColor;

		#else

			out_Diffuse = vFinalColor;
			out_Lightmap = vec4(1.0, 1.0, 1.0, 1.0);
			out_WorldNorm = vec4(vOctNormal.x, vOctNormal.y, flDistanceToFragment, out_Diffuse.a);
			out_Specular = specularColor;

		#endif

	#else

		vec4 finalColor = CalcFog(
			ProcessLinearBlendShift(vFinalColor),
			flDistanceToFragment
		);

		out_Diffuse = finalColor;

	#endif

#endif
}