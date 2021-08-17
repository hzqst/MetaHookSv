#version 130

uniform mat4 entityMatrix;
uniform vec3 viewpos;
uniform float speed;

varying vec3 worldpos;
varying vec3 normal;
varying vec3 tangent;
varying vec3 bitangent;
varying vec4 color;

attribute vec3 s_tangent;
attribute vec3 t_tangent;

#ifdef SHADOWMAP_ENABLED

uniform mat4 shadowMatrix[3];
varying vec4 shadowcoord[3];

#endif

void main(void)
{
	vec4 worldpos4 = entityMatrix * gl_Vertex;
    worldpos = worldpos4.xyz;

	vec4 normal4 = vec4(gl_Normal, 0.0);
	normal = normalize(entityMatrix * normal4).xyz;

#ifdef DIFFUSE_ENABLED
	gl_TexCoord[0] = vec4(gl_MultiTexCoord0.x + gl_MultiTexCoord0.z * speed, gl_MultiTexCoord0.y, 0.0, 0.0);
#endif

#ifdef LIGHTMAP_ENABLED
	gl_TexCoord[1] = gl_MultiTexCoord1;
#endif

#ifdef DETAILTEXTURE_ENABLED
	gl_TexCoord[2] = gl_MultiTexCoord2;
#endif

#if defined(NORMALTEXTURE_ENABLED) || defined(PARALLAXTEXTURE_ENABLED)
    vec4 tangent4 = vec4(s_tangent, 0.0);
    tangent = normalize((entityMatrix * tangent4).xyz);
	vec4 bitangent4 = vec4(t_tangent, 0.0);
    bitangent = normalize((entityMatrix * bitangent4).xyz);
#endif

#ifdef NORMALTEXTURE_ENABLED
	gl_TexCoord[3] = gl_MultiTexCoord3;
#endif

#ifdef PARALLAXTEXTURE_ENABLED
	gl_TexCoord[4] = gl_MultiTexCoord4;
#endif

#ifdef SPECULARTEXTURE_ENABLED
	gl_TexCoord[5] = gl_MultiTexCoord5;
#endif

#ifdef SHADOWMAP_ENABLED

	#ifdef SHADOWMAP_HIGH_ENABLED
        shadowcoord[0] = shadowMatrix[0] * vec4(worldpos, 1.0);
    #endif

    #ifdef SHADOWMAP_MEDIUM_ENABLED
        shadowcoord[1] = shadowMatrix[1] * vec4(worldpos, 1.0);
    #endif

    #ifdef SHADOWMAP_LOW_ENABLED
        shadowcoord[2] = shadowMatrix[2] * vec4(worldpos, 1.0);
    #endif

#endif

	color = gl_Color;

	gl_Position = ftransform();
}