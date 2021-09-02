#version 430

uniform sampler2D baseTex;
uniform sampler2D lumTex;

in vec2 texCoord;

in float lum;

layout(location=0) out vec4 out_Color;

void main()
{
	vec4 vColor = texture(baseTex, texCoord.xy);
	float fLumValue = dot(vec3(0.27, 0.67, 0.06), vColor.rgb);
	
	out_Color = max(vColor * (fLumValue - lum) * 5.0, vec4(0.0, 0.0, 0.0, 1.0));
}