#version 120
uniform vec3 eyepos;
varying vec4 projnormal;
varying vec2 refractcoord;
varying vec3 worldnormal;
varying vec3 worldvec;

void main(void)
{
	vec3 vWorldPos = gl_Vertex.xyz / gl_Vertex.w;
	worldvec = normalize(vWorldPos - eyepos);
	worldnormal = normalize(gl_Normal.xyz);
	
	vec4 vProjPos = gl_ModelViewProjectionMatrix * gl_Vertex;

	vec4 vNormalEnd = vec4(vWorldPos + worldnormal, 1.0);
	projnormal = gl_ModelViewProjectionMatrix * vNormalEnd;
	
	refractcoord = vProjPos.xy / vProjPos.w;
	
	projnormal.xy = projnormal.xy / projnormal.w - refractcoord;

	refractcoord = refractcoord * 0.5 + 0.5;
	
	gl_Position = ftransform();
}