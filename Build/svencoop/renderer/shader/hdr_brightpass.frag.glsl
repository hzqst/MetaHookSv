#version 430

layout(binding=0) uniform sampler2D baseTex;
layout(binding=1) uniform sampler2D lumTex;

layout(location=0) uniform float exposure;

in vec2 texCoord;

layout(location=0) out vec4 out_Color;

float SampleLumTexture()
{
	return texture(lumTex, vec2(0.5, 0.5)).x;
}

float CalcLum()
{
	return min(SampleLumTexture(), 0.8);
}

void main()
{
	vec4 vColor = texture(baseTex, texCoord.xy);
	float fLumValue = dot(vec3(0.27, 0.67, 0.06), vColor.rgb);

	out_Color = max(vColor * (fLumValue - CalcLum()) * 5.0, vec4(0.0, 0.0, 0.0, 1.0));
}