#version 120

uniform sampler2D depthmap_high;
uniform sampler2D depthmap_medium;
uniform sampler2D depthmap_low;

uniform sampler2D posmap_high;
uniform sampler2D posmap_medium;
uniform sampler2D posmap_low;

uniform float texoffset_high = 1.0 / 1024.0;
uniform float texoffset_medium = 1.0 / 1024.0;
uniform float texoffset_low = 1.0 / 1024.0;

uniform int numedicts_high = 0;
uniform int numedicts_medium = 0;
uniform int numedicts_low = 0;

uniform float alpha = 1.0;
uniform vec2 fade;

varying vec4 shadowcoord_high;
varying vec4 shadowcoord_medium;
varying vec4 shadowcoord_low;

varying vec4 scene_worldpos;

float lookup_high(vec4 coord, vec2 off)
{
	vec4 shadowCoord = coord + vec4(off.x * texoffset_high, off.y * texoffset_high, 0.0, 0.0);
	float depth = texture2D(depthmap_high, shadowCoord.xy / shadowCoord.w).x;
	float depth1 = shadowCoord.z / shadowCoord.w;
	return depth < depth1 ? 0.0 : 1.0;
}

vec3 getpos_high(vec4 coord)
{
	return texture2D(posmap_high, coord.xy / coord.w).xyz;
}

float lookup_medium(vec4 coord, vec2 off)
{
	vec4 shadowCoord = coord + vec4(off.x * texoffset_medium, off.y * texoffset_medium, 0.0, 0.0);
	float depth = texture2D(depthmap_medium, shadowCoord.xy / shadowCoord.w).x;
	float depth1 = shadowCoord.z / shadowCoord.w;
	return depth < depth1 ? 0.0 : 1.0;
}

vec3 getpos_medium(vec4 coord)
{
	return texture2D(posmap_medium, coord.xy / coord.w).xyz;
}

float lookup_low(vec4 coord, vec2 off)
{
	vec4 shadowCoord = coord + vec4(off.x * texoffset_low, off.y * texoffset_medium, 0.0, 0.0);
	float depth = texture2D(depthmap_low, shadowCoord.xy / shadowCoord.w).x;
	float depth1 = shadowCoord.z / shadowCoord.w;
	return depth < depth1 ? 0.0 : 1.0;
}

vec3 getpos_low(vec4 coord)
{
	return texture2D(posmap_low, coord.xy / coord.w).xyz;
}

void main()
{
	// Gaussian 3x3 filter
	float shadow_high = 0;
	if(numedicts_high > 0){
		shadow_high = 0;
		shadow_high += lookup_high(shadowcoord_high, vec2(0.0,0.0)) * 0.25;
		shadow_high += lookup_high(shadowcoord_high, vec2( -1.0, -1.0)) * 0.0625;
		shadow_high += lookup_high(shadowcoord_high, vec2( -1.0, 0.0)) * 0.125;
		shadow_high += lookup_high(shadowcoord_high, vec2( -1.0, 1.0)) * 0.0625;
		shadow_high += lookup_high(shadowcoord_high, vec2( 0.0, -1.0)) * 0.125;
		shadow_high += lookup_high(shadowcoord_high, vec2( 0.0, 1.0)) * 0.125;
		shadow_high += lookup_high(shadowcoord_high, vec2( 1.0, -1.0)) * 0.0625;
		shadow_high += lookup_high(shadowcoord_high, vec2( 1.0, 0.0)) * 0.125;
		shadow_high += lookup_high(shadowcoord_high, vec2( 1.0, 1.0)) * 0.0625;

	}

	float shadow_medium = 1;
	if(numedicts_medium > 0){
		shadow_medium = 0;
		shadow_medium += lookup_medium(shadowcoord_medium, vec2(0.0,0.0));
		shadow_medium += lookup_medium(shadowcoord_medium, vec2(0.035,0.0));
		shadow_medium += lookup_medium(shadowcoord_medium, vec2(-0.035,0.0));
		shadow_medium += lookup_medium(shadowcoord_medium, vec2(0.0,0.035));
		shadow_medium += lookup_medium(shadowcoord_medium, vec2(0.0,-0.035));
		shadow_medium *= 0.2;
	}

	float shadow_low = 1;
	if(numedicts_low > 0){
		shadow_low += lookup_low(shadowcoord_low, vec2(0.0,0.0));
		shadow_low += lookup_low(shadowcoord_low, vec2(0.035,0.0));
		shadow_low += lookup_low(shadowcoord_low, vec2(-0.035,0.0));
		shadow_low += lookup_low(shadowcoord_low, vec2(0.0,0.035));
		shadow_low += lookup_low(shadowcoord_low, vec2(0.0,-0.035));
		shadow_low *= 0.2;
	}
	
	// > 0.95 means not blocked by shadow caster
	if(shadow_high > 0.95 && shadow_medium > 0.95 && shadow_low > 0.95)
	{
		discard;
		//gl_FragColor = vec4(0, 0, 0, 0);
	}
	else
	{
		float final_alpha = alpha;

		if(shadow_high < 0.95){
			vec3 scene = scene_worldpos.xyz;
			vec3 caster = getpos_high(shadowcoord_high);

			float dist = distance(caster, scene);
			float fade_val = (dist - fade.x) / fade.y;
			final_alpha *= 1.0 - clamp(fade_val, 0.0, 1.0);

		} else if(shadow_medium < 0.95) {
			vec3 scene = scene_worldpos.xyz;
			vec3 caster = getpos_medium(shadowcoord_medium);

			float dist = distance(caster, scene);
			float fade_val = (dist - fade.x) / fade.y;
			final_alpha *= 1.0 - clamp(fade_val, 0.0, 1.0);
		} else if(shadow_low < 0.95) {
			vec3 scene = scene_worldpos.xyz;
			vec3 caster = getpos_low(shadowcoord_low);

			float dist = distance(caster, scene);
			float fade_val = (dist - fade.x) / fade.y;
			final_alpha *= 1.0 - clamp(fade_val, 0.0, 1.0);
		}

		shadow_high = 1.0 - shadow_high;
		shadow_medium = 1.0 - shadow_medium;
		shadow_low = 1.0 - shadow_low;

		float shadow_total = shadow_high + shadow_medium + shadow_low;

		gl_FragColor = vec4(0, 0, 0, shadow_total * final_alpha);
	}
}