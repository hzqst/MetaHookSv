//fragment shader
#version 120
uniform sampler2D refract;
uniform float cloakfactor;
uniform float refractamount;
varying vec4 projnormal;
varying vec2 refractcoord;
varying vec3 worldnormal;
varying vec3 worldvec;

const vec4 g_vPoissonOffset_1 = vec4(-0.0876,  0.9703,  0.5651,  0.4802);
const vec4 g_vPoissonOffset_2 = vec4(0.1851,  0.1580, -0.0617, -0.2616);
const vec4 g_vPoissonOffset_3 = vec4(-0.5477, -0.6603,  0.0711, -0.5325);
const vec4 g_vPoissonOffset_4 = vec4(-0.0751, -0.8954,  0.4054,  0.6384);

void main(void)
{
	vec3 vWorldNormal = normalize(worldnormal);
	
	vec2 vRefractTexCoord = projnormal.xy * (refractamount - refractamount * cloakfactor) + refractcoord;

	// Blur by scalable Poisson filter
	float flBlurAmount = 0.05 - 0.05 * cloakfactor;
	vec4 cRefract = texture2D(refract, vRefractTexCoord);
	cRefract += texture2D( refract, vRefractTexCoord + ( g_vPoissonOffset_1.xy * flBlurAmount ) );
	cRefract += texture2D( refract, vRefractTexCoord + ( g_vPoissonOffset_1.wz * flBlurAmount ) );
	cRefract += texture2D( refract, vRefractTexCoord + ( g_vPoissonOffset_2.xy * flBlurAmount ) );
	cRefract += texture2D( refract, vRefractTexCoord + ( g_vPoissonOffset_2.wz * flBlurAmount ) );
	cRefract += texture2D( refract, vRefractTexCoord + ( g_vPoissonOffset_3.xy * flBlurAmount ) );
	cRefract += texture2D( refract, vRefractTexCoord + ( g_vPoissonOffset_3.wz * flBlurAmount ) );
	cRefract += texture2D( refract, vRefractTexCoord + ( g_vPoissonOffset_4.xy * flBlurAmount ) );
	cRefract += texture2D( refract, vRefractTexCoord + ( g_vPoissonOffset_4.wz * flBlurAmount ) );
	cRefract /= 9.0f;
	
	// 1-(N.V) for Fresnel term (NOTE: If this math changes, you need to update the C code that mimics this on the CPU)
	float flFresnel = 1.0f - clamp( dot( vWorldNormal, normalize(-worldvec) ), 0.0, 1.0 );

	float flCloakLerpFactor = clamp( 1.0f + (flFresnel - 2.35f) * cloakfactor, 0.0, 1.0 );

	flCloakLerpFactor = 1.0f - smoothstep( 0.4f, 0.425f, flCloakLerpFactor );
	
	// Slightly dim the facing pixels and brighten the silhouette pixels
	float flRefractLerp = flFresnel * 0.4 + 0.8;
	cRefract.rgb *= flRefractLerp + (1.0 - flRefractLerp) * cloakfactor * cloakfactor;
	// This gives a scalar in the range [0.8 1.2]

	gl_FragColor.rgb = cRefract.rgb;
	gl_FragColor.a = flCloakLerpFactor;
}