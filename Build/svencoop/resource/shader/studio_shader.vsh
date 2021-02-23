
uniform mat3x4 bonematrix[128];
uniform float v_lambert;
uniform float v_brightness;
uniform float v_lightgamma;
uniform float r_ambientlight;
uniform float r_shadelight;
uniform float r_blend;
uniform float r_g1;
uniform float r_g3;
uniform vec3 r_plightvec;
uniform vec3 r_colormix;

attribute ivec2 attrbone;

varying vec4 worldpos;
varying vec4 normal;
varying vec4 color;

void main(void)
{
	vec3 vert = gl_Vertex.xyz;
	vec3 norm = gl_Normal;

	int vertbone = attrbone.x;
	int normbone = attrbone.y;

	mat3x4 vertbone_matrix = bonematrix[vertbone];
	vec3 outvert = vec3(
		dot(vert, vertbone_matrix[0]) + vertbone_matrix[0][3],
		dot(vert, vertbone_matrix[1]) + vertbone_matrix[1][3],
		dot(vert, vertbone_matrix[2]) + vertbone_matrix[2][3]
	);

	mat3x4 normbone_matrix = bonematrix[normbone];
	vec3 outnorm = vec3(
		dot(norm, normbone_matrix[0]),
		dot(norm, normbone_matrix[1]),
		dot(norm, normbone_matrix[2])
	);

	worldpos = vec4(outvert, 1.0);
	normal = vec4(normalize(outnorm), 1.0);

	float lv = 1.0;

#ifdef STUDIO_FULLBRIGHT

	color = vec4(1.0, 1.0, 1.0, r_blend);

#else

	float illum = r_ambientlight;

	#ifdef STUDIO_FLATSHADE

		illum += r_shadelight * 0.8;

	#else

		float lightcos = max(dot(normal.xyz, r_plightvec), 1.0);

		illum += r_shadelight;

		float r = v_lambert;
		if(r > 1.0)
		{
			lightcos = (lightcos + r - 1.0) / r;
		}
		else
		{
			r += 1.0;
			lightcos = ((r - 1.0) - lightcos) / r; 
		}
		
		illum -= r_shadelight * max(lightcos, 0.0); 

		illum = clamp(illum, 0.0, 255.0);

	#endif

	float fv = (illum * 4.0) / 1023.0;
	fv = pow(fv, v_lightgamma);

	fv = fv * max(v_brightness, 1.0);

	if (fv < r_g3)
		fv = (fv / r_g3) * 0.125;
	else 
		fv = 0.125 + ((fv - r_g3) / (1.0 - r_g3)) * 0.875;

	float inf = 1023.0 * pow( fv, r_g1 );

	inf = clamp(inf, 0.0, 1023.0);

	lv = inf / 1023.0;

	color = vec4(lv * r_colormix.x, lv * r_colormix.y, lv * r_colormix.z, r_blend);
#endif

	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(outvert, 1.0);
}