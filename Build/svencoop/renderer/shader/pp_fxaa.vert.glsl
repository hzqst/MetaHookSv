#version 120

varying vec4 posPos;
uniform float FXAA_SUBPIX_SHIFT = 1.0/4.0;
uniform float rt_w; // GeeXLab built-in
uniform float rt_h; // GeeXLab built-in

void main(void)
{
	gl_Position = ftransform();
	gl_TexCoord[0] = gl_MultiTexCoord0;
	vec2 rcpFrame = vec2(1.0/rt_w, 1.0/rt_h);
	posPos.xy = gl_MultiTexCoord0.xy;
	posPos.zw = gl_MultiTexCoord0.xy - (rcpFrame * (0.5 + FXAA_SUBPIX_SHIFT));
}