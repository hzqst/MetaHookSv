#version 130

uniform mat4 entityMatrix;

uniform float speed;
varying vec4 worldpos;
varying vec4 normal;
varying vec4 tangent;
varying vec4 color;

attribute vec3 s_tangent;
attribute vec3 t_tangent;

#ifdef PARALLAXTEXTURE_ENABLED

uniform vec4 viewpos;

varying vec3 tangentViewPos;
varying vec3 tangentFragPos;

#endif

#ifdef SHADOWMAP_ENABLED

uniform mat4 shadowMatrix[3];
varying vec4 shadowcoord[3];

#endif

mat3 inverse_mat3(mat3 m)
{
    float Determinant = 
          m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2])
        - m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2])
        + m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]);
    
    mat3 Inverse;
    Inverse[0][0] = + (m[1][1] * m[2][2] - m[2][1] * m[1][2]);
    Inverse[1][0] = - (m[1][0] * m[2][2] - m[2][0] * m[1][2]);
    Inverse[2][0] = + (m[1][0] * m[2][1] - m[2][0] * m[1][1]);
    Inverse[0][1] = - (m[0][1] * m[2][2] - m[2][1] * m[0][2]);
    Inverse[1][1] = + (m[0][0] * m[2][2] - m[2][0] * m[0][2]);
    Inverse[2][1] = - (m[0][0] * m[2][1] - m[2][0] * m[0][1]);
    Inverse[0][2] = + (m[0][1] * m[1][2] - m[1][1] * m[0][2]);
    Inverse[1][2] = - (m[0][0] * m[1][2] - m[1][0] * m[0][2]);
    Inverse[2][2] = + (m[0][0] * m[1][1] - m[1][0] * m[0][1]);
    Inverse /= Determinant;
    
    return Inverse;
}

void main(void)
{
	worldpos = entityMatrix * gl_Vertex;
	normal = vec4(gl_Normal, 0.0);
	normal = normalize(entityMatrix * normal);

#ifdef NORMALTEXTURE_ENABLED
    tangent = vec4(s_tangent, 0.0);
    tangent = normalize(entityMatrix * tangent);
#endif

#ifdef DIFFUSE_ENABLED
	gl_TexCoord[0] = vec4(gl_MultiTexCoord0.x + gl_MultiTexCoord0.z * speed, gl_MultiTexCoord0.y, 0.0, 0.0);
#endif

#ifdef LIGHTMAP_ENABLED
	gl_TexCoord[1] = gl_MultiTexCoord1;
#endif

#ifdef DETAILTEXTURE_ENABLED
	gl_TexCoord[2] = gl_MultiTexCoord2;
#endif

#ifdef NORMALTEXTURE_ENABLED
	gl_TexCoord[3] = gl_MultiTexCoord3;
#endif

#ifdef PARALLAXTEXTURE_ENABLED
	gl_TexCoord[4] = gl_MultiTexCoord4;

    //world to tangent
	mat3 normalMatrix = transpose(inverse_mat3(mat3(entityMatrix)));
    vec3 T = normalize(normalMatrix * s_tangent);
    vec3 N = normalize(normalMatrix * gl_Normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

	mat3 TBN = transpose(mat3(T, B, N));    
    tangentViewPos = TBN * viewpos.xyz;
    tangentFragPos = TBN * worldpos.xyz;
#endif

#ifdef SHADOWMAP_ENABLED

	#ifdef SHADOWMAP_HIGH_ENABLED
        shadowcoord[0] = shadowMatrix[0] * gl_ModelViewMatrix * gl_Vertex;
    #endif

    #ifdef SHADOWMAP_MEDIUM_ENABLED
        shadowcoord[1] = shadowMatrix[1] * gl_ModelViewMatrix * gl_Vertex;
    #endif

    #ifdef SHADOWMAP_LOW_ENABLED
        shadowcoord[2] = shadowMatrix[2] * gl_ModelViewMatrix * gl_Vertex;
    #endif

#endif

	color = gl_Color;

	gl_Position = ftransform();
}