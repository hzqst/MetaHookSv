#version 120
uniform sampler2DShadow depthmap_high;
uniform sampler2DShadow depthmap_medium;
uniform sampler2DShadow depthmap_low;
uniform float texoffset_high = 1.0 / 1024.0;
uniform float texoffset_medium = 1.0 / 1024.0;
uniform float texoffset_low = 1.0 / 1024.0;
uniform float alpha = 1.0;

varying vec4 shadowcoord_high;
varying vec4 shadowcoord_medium;
varying vec4 shadowcoord_low;
//varying vec3 worldpos;

float lookup_high(vec4 coord, vec2 off)
{
	return shadow2DProj(depthmap_high, coord + vec4(off.x * texoffset_high, off.y * texoffset_high, 0.0, 0.0) ).w;
}

float lookup_medium(vec4 coord, vec2 off)
{
	return shadow2DProj(depthmap_medium, coord + vec4(off.x * texoffset_medium, off.y * texoffset_medium, 0.0, 0.0) ).w;
}

float lookup_low(vec4 coord, vec2 off)
{
	return shadow2DProj(depthmap_low, coord + vec4(off.x * texoffset_low, off.y * texoffset_low, 0.0, 0.0) ).w;
}

void main()
{
	// Gaussian 3x3 filter
	float shadow_high = lookup_high(shadowcoord_high, vec2(0.0,0.0)) * 0.25;
	shadow_high += lookup_high(shadowcoord_high, vec2( -1.0, -1.0)) * 0.0625;
	shadow_high += lookup_high(shadowcoord_high, vec2( -1.0, 0.0)) * 0.125;
	shadow_high += lookup_high(shadowcoord_high, vec2( -1.0, 1.0)) * 0.0625;
	shadow_high += lookup_high(shadowcoord_high, vec2( 0.0, -1.0)) * 0.125;
	shadow_high += lookup_high(shadowcoord_high, vec2( 0.0, 1.0)) * 0.125;
	shadow_high += lookup_high(shadowcoord_high, vec2( 1.0, -1.0)) * 0.0625;
	shadow_high += lookup_high(shadowcoord_high, vec2( 1.0, 0.0)) * 0.125;
	shadow_high += lookup_high(shadowcoord_high, vec2( 1.0, 1.0)) * 0.0625;

	float shadow_medium = 
	lookup_medium(shadowcoord_medium, vec2(0.0,0.0)) + 
	lookup_medium(shadowcoord_medium, vec2(0.035,0.0)) + 
	lookup_medium(shadowcoord_medium, vec2(-0.035,0.0)) + 
	lookup_medium(shadowcoord_medium, vec2(0.0,0.035)) + 
	lookup_medium(shadowcoord_medium, vec2(0.0,-0.035));

	shadow_medium *= 0.2;

	float shadow_low = 
	lookup_low(shadowcoord_low, vec2(0.0,0.0)) + 
	lookup_low(shadowcoord_low, vec2(0.035,0.0)) + 
	lookup_low(shadowcoord_low, vec2(-0.035,0.0)) + 
	lookup_low(shadowcoord_low, vec2(0.0,0.035)) + 
	lookup_low(shadowcoord_low, vec2(0.0,-0.035));

	shadow_low *= 0.2;
	
	if(shadow_high > 0.95 && shadow_medium > 0.95 && shadow_low > 0.95)
		discard;

	shadow_high = 1.0-shadow_high;
	shadow_medium = 1.0-shadow_medium;
	shadow_low = 1.0-shadow_low;

	float shadow = shadow_high + shadow_medium + shadow_low;

	gl_FragColor = vec4(0, 0, 0, shadow * alpha);

	//for debugging
	//gl_FragColor = vec4(shadow_high, shadow_medium, shadow_low, 0.5);
}