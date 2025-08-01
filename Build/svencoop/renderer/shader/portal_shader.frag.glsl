#version 430

#include "common.h"

uniform mat4 u_entityMatrix;

layout(binding = 0) uniform sampler2D portalTex;
layout(binding = 1) uniform sampler2D overlayTex;

in vec4 v_projpos;
in vec3 v_worldpos;
in vec3 v_normal;
in vec2 v_diffusetexcoord;

layout(location = 0) out vec4 out_Diffuse;

void main()
{
	vec4 vFinalColor = vec4(0.0);

#if defined(PORTAL_TEXCOORD_ENABLED)

	vec2 vPortalTexCoord = v_projpos.xy / v_projpos.w * 0.5 + 0.5;

#elif defined(REVERSE_PORTAL_TEXCOORD_ENABLED)

	vec2 vPortalTexCoord = v_projpos.xy / v_projpos.w * 0.5 + 0.5;
	vPortalTexCoord.x = -vPortalTexCoord.x;

#else

	vec2 vPortalTexCoord = vec2(v_diffusetexcoord.x, -v_diffusetexcoord.y);//v_projpos.xy / v_projpos.w * 0.5 + 0.5;

#endif

	vec4 vPortalColor = texture(portalTex, vPortalTexCoord);
	
	vPortalColor = ProcessOtherGammaColor(vPortalColor);

	vFinalColor = vPortalColor;

	float flDistanceToFragment = distance(v_worldpos.xyz, CameraUBO.viewpos.xyz);

#if defined(PORTAL_OVERLAY_TEXTURE_ENABLED)

	vec2 vOverlayTexCoord = vec2(v_diffusetexcoord.x, v_diffusetexcoord.y);
	vec4 vOverlayColor = texture(overlayTex, vOverlayTexCoord);

	vOverlayColor = ProcessDiffuseColor(vOverlayColor);

	if(vOverlayColor.a > 0.5)
		vFinalColor = vOverlayColor;

#endif

	vec4 finalColor = CalcFog(
		ProcessLinearBlendShift(vFinalColor),
		flDistanceToFragment
	);

	out_Diffuse = finalColor;
}