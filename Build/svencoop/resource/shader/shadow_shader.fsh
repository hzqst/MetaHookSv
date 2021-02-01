#version 120
uniform sampler2DShadow depthmap;
uniform float texoffset = 1.0 / 512.0;
uniform float radius;
uniform float fard;
uniform vec3 entorigin;

varying vec4 ShadowCoord;
varying vec3 worldpos;

float lookup(vec4 coord, vec2 off)
{
	return shadow2DProj(depthmap, coord + vec4(off.x * texoffset, off.y * texoffset, 0.0, 0.0) ).w;
}

void main()
{
	float shadow = lookup(ShadowCoord, vec2(0.0,0.0)) + lookup(ShadowCoord, vec2(0.035,0.0)) + lookup(ShadowCoord, vec2(-0.035,0.0)) + lookup(ShadowCoord, vec2(0.0,0.035)) + lookup(ShadowCoord, vec2(0.0,-0.035));
	shadow *= 0.2;

	// Gaussian 3x3 filter
	/*float shadow = lookup(ShadowCoord, vec2(0.0,0.0)) * 0.25;
	shadow += lookup(ShadowCoord, vec2( -1.0, -1.0)) * 0.0625;
	shadow += lookup(ShadowCoord, vec2( -1.0, 0.0)) * 0.125;
	shadow += lookup(ShadowCoord, vec2( -1.0, 1.0)) * 0.0625;
	shadow += lookup(ShadowCoord, vec2( 0.0, -1.0)) * 0.125;
	shadow += lookup(ShadowCoord, vec2( 0.0, 1.0)) * 0.125;
	shadow += lookup(ShadowCoord, vec2( 1.0, -1.0)) * 0.0625;
	shadow += lookup(ShadowCoord, vec2( 1.0, 0.0)) * 0.125;
	shadow += lookup(ShadowCoord, vec2( 1.0, 1.0)) * 0.0625;*/
	
	if(shadow > 0.9)
		discard;

	float dist = length(worldpos - entorigin);
	if(dist > radius)
		discard;
		
	shadow = 1.0-shadow;

	if(dist > fard)
	{
		shadow = clamp(shadow * (1.0 - (dist - fard) / (radius - fard)), 0.1, 1.0);
	}

	gl_FragColor = vec4(0.0, 0.0, 0.0, shadow * 0.5);
}