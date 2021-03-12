//Airblast Fragment Shader by hzqst

uniform sampler2D normalmap;
uniform sampler2D refractmap;
uniform vec2 packedfactor;
varying vec4 projpos;

const vec4 g_vPoissonOffset_1 = vec4(-0.0876,  0.9703,  0.5651,  0.4802);
const vec4 g_vPoissonOffset_2 = vec4(0.1851,  0.1580, -0.0617, -0.2616);
const vec4 g_vPoissonOffset_3 = vec4(-0.5477, -0.6603,  0.0711, -0.5325);
const vec4 g_vPoissonOffset_4 = vec4(-0.0751, -0.8954,  0.4054,  0.6384);

void main()
{
	//calculate the normal texcoord and sample the normal vector from texture
	vec4 vNormal = texture2D(normalmap, gl_TexCoord[0].xy);
	//unpack the normal from rgb into vector
	vNormal.xyz = vNormal.xyz * 2.0 - 1.0;
	
	//calculate the edgefactor, the more close to the center, the more blur it is
	//vec2 vEdgeFactor = vec2(gl_TexCoord[0].x - 0.5, gl_TexCoord[0].y - 0.5);
	//float flEdgeFactor = (0.5 - (vEdgeFactor.x * vEdgeFactor.x + vEdgeFactor.y * vEdgeFactor.y)) * 2.0;

	//flEdgeFactor = clamp(flEdgeFactor, 0.0, 1.0);
	//unpacke the packedfactor
	float flRefractFactor = packedfactor.y;// * vEdgeFactor;
	float flBlurFactor = packedfactor.x;// * flEdgeFactor;

	//calculate texcoord
	vec2 vBaseTexCoord = projpos.xy / projpos.w * 0.5 + 0.5;
	vec2 vOffsetTexCoord = normalize(vNormal.xyz).xy * flRefractFactor;

	//sample the refract color
	vec2 vRefractTexCoord = vBaseTexCoord + vOffsetTexCoord;
	vec4 vRefractColor = texture2D(refractmap, vRefractTexCoord);
	
	//blur sampler
	if(flBlurFactor != 0.0)
	{
		vRefractColor += texture2D( refractmap, vRefractTexCoord + ( g_vPoissonOffset_1.xy * flBlurFactor ) );
		vRefractColor += texture2D( refractmap, vRefractTexCoord + ( g_vPoissonOffset_1.wz * flBlurFactor ) );
		vRefractColor += texture2D( refractmap, vRefractTexCoord + ( g_vPoissonOffset_2.xy * flBlurFactor ) );
		vRefractColor += texture2D( refractmap, vRefractTexCoord + ( g_vPoissonOffset_2.wz * flBlurFactor ) );
		vRefractColor += texture2D( refractmap, vRefractTexCoord + ( g_vPoissonOffset_3.xy * flBlurFactor ) );
		vRefractColor += texture2D( refractmap, vRefractTexCoord + ( g_vPoissonOffset_3.wz * flBlurFactor ) );
		vRefractColor += texture2D( refractmap, vRefractTexCoord + ( g_vPoissonOffset_4.xy * flBlurFactor ) );
		vRefractColor += texture2D( refractmap, vRefractTexCoord + ( g_vPoissonOffset_4.wz * flBlurFactor ) );
		vRefractColor /= 9.0f;
	}
	
	//get alpha from normal texture
	vRefractColor.a = vNormal.a;

	gl_FragColor = vRefractColor * gl_Color;
}