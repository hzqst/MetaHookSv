//Water Fragment Shader by hzqst

uniform vec3 eyepos;
uniform float time;
uniform sampler2D basemap;
uniform sampler2D normalmap;

void main()
{
	//calculate the normal texcoord and sample the normal vector from texture
	vec2 vNormTexCoord1 = vec2(0.2, 0.15) * time + gl_TexCoord[1].xy;
	vec2 vNormTexCoord2 = vec2(-0.13, 0.11) * time + gl_TexCoord[1].xy;
	vec2 vNormTexCoord3 = vec2(-0.14, -0.16) * time + gl_TexCoord[1].xy;
	vec2 vNormTexCoord4 = vec2(0.17, 0.15) * time + gl_TexCoord[1].xy;
	vec4 vNorm1 = texture2D(normalmap, vNormTexCoord1);
	vec4 vNorm2 = texture2D(normalmap, vNormTexCoord2);
	vec4 vNorm3 = texture2D(normalmap, vNormTexCoord3);
	vec4 vNorm4 = texture2D(normalmap, vNormTexCoord4);
	vec4 vNormal = vNorm1 + vNorm2 + vNorm3 + vNorm4;
	vNormal = (vNormal / 4.0) * 2.0 - 1.0;

	//calculate texcoord
	vec2 vBaseTexCoord = gl_TexCoord[0].xy;
	vec2 vOffsetTexCoord = normalize(vNormal.xyz).xy * 0.3;

	//sample the basetexture color
	vec2 vResultTexCoord = vBaseTexCoord + vOffsetTexCoord;
	vec4 vResultColor = texture2D(basemap, vResultTexCoord);

	gl_FragColor = vResultColor;
}