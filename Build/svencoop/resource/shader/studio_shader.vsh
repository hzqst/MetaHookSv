uniform vec3 lightpos;
uniform vec3 eyepos;
varying vec3 lightvec;
varying vec3 halfvec;
attribute vec3 tangent;
attribute vec3 binormal;

void main(void)
{
	vec4 pos = vec4(gl_Vertex.xyz / gl_Vertex.w, 1.0);
	pos = gl_ModelViewMatrix * pos;

	vec4 vlightpos = (gl_ModelViewMatrix * vec4(lightpos, 1.0));
	vec4 veyepos = (gl_ModelViewMatrix * vec4(eyepos, 1.0));

	vec3 lightdir = normalize(vlightpos.xyz - pos.xyz);
	vec3 eyedir = normalize(veyepos.xyz - pos.xyz);

	vec3 n = normalize(gl_NormalMatrix * gl_Normal);
	vec3 t = normalize(gl_NormalMatrix * tangent);
	vec3 b = normalize(gl_NormalMatrix * binormal);

	vec3 halfdir = normalize(lightdir + eyedir);

	lightvec.x = dot(t, lightdir);
	lightvec.y = dot(b, lightdir);
	lightvec.z = dot(n, lightdir);
	lightvec = normalize(lightvec);

	vec3 eyevec;
	eyevec.x = dot(t, eyedir);
	eyevec.y = dot(b, eyedir);
	eyevec.z = dot(n, eyedir);
	eyevec = normalize(eyevec);

	halfvec = normalize(lightvec + eyevec);

	gl_FrontColor = gl_Color;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
}