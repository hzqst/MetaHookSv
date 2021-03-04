#version 130
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
uniform vec3 r_origin;
uniform vec3 r_vright;
uniform float r_scale;

attribute ivec2 attr_bone;

varying vec4 worldpos;
varying vec4 normal;
varying vec4 color;

void main(void)
{
	vec3 vert = gl_Vertex.xyz;
	vec3 norm = gl_Normal;

	int vertbone = attr_bone.x;
	int normbone = attr_bone.y;

	mat3x4 vertbone_matrix = bonematrix[vertbone];
    vec3 vertbone_matrix_0 = vec3(vertbone_matrix[0][0], vertbone_matrix[0][1], vertbone_matrix[0][2]);
  	vec3 vertbone_matrix_1 = vec3(vertbone_matrix[1][0], vertbone_matrix[1][1], vertbone_matrix[1][2]);
    vec3 vertbone_matrix_2 = vec3(vertbone_matrix[2][0], vertbone_matrix[2][1], vertbone_matrix[2][2]);
	vec3 outvert = vec3(
		dot(vert, vertbone_matrix_0) + vertbone_matrix[0][3],
		dot(vert, vertbone_matrix_1) + vertbone_matrix[1][3],
		dot(vert, vertbone_matrix_2) + vertbone_matrix[2][3]
	);

	mat3x4 normbone_matrix = bonematrix[normbone];
	vec3 normbone_matrix_0 = vec3(normbone_matrix[0][0], normbone_matrix[0][1], normbone_matrix[0][2]);
    vec3 normbone_matrix_1 = vec3(normbone_matrix[1][0], normbone_matrix[1][1], normbone_matrix[1][2]);
    vec3 normbone_matrix_2 = vec3(normbone_matrix[2][0], normbone_matrix[2][1], normbone_matrix[2][2]);
	vec3 outnorm = vec3(
		dot(norm, normbone_matrix_0),
		dot(norm, normbone_matrix_1),
		dot(norm, normbone_matrix_2)
	);

	outnorm = normalize(outnorm);

	worldpos = vec4(outvert, 1.0);
	normal = vec4(outnorm, 1.0);

#ifdef STUDIO_NF_FULLBRIGHT

	color = vec4(1.0, 1.0, 1.0, r_blend);

#else

	float illum = r_ambientlight;

	#ifdef STUDIO_NF_FLATSHADE

		illum += r_shadelight * 0.8;

	#else

		float lightcos = dot(outnorm, r_plightvec);

		if(v_lambert < 1.0)
		{
			lightcos = (v_lambert - lightcos) / (v_lambert + 1.0); 
			illum += r_shadelight * max(lightcos, 0.0); 			
		}
		else
		{
			illum += r_shadelight;
			lightcos = (lightcos + v_lambert - 1.0) / v_lambert;
			illum -= r_shadelight * max(lightcos, 0.0);
		}

	#endif

	illum = clamp(illum, 0.0, 255.0);

	float fv = illum / 255.0;
	fv = pow(fv, v_lightgamma);

	fv = fv * max(v_brightness, 1.0);

	if (fv > r_g3)
		fv = 0.125 + ((fv - r_g3) / (1.0 - r_g3)) * 0.875;
	else 
		fv = (fv / r_g3) * 0.125;

	float lv = clamp(pow( fv, r_g1 ), 0.0, 1.0);

	color = vec4(lv * r_colormix.x, lv * r_colormix.y, lv * r_colormix.z, r_blend);

#endif

#ifdef STUDIO_NF_CHROME

	outvert = outvert + outnorm * r_scale;
	worldpos = vec4(outvert, 1.0);

	vec3 tmp = vec3(
		normbone_matrix[0][3] - r_origin.x,
		normbone_matrix[1][3] - r_origin.y,
		normbone_matrix[2][3] - r_origin.z
	);
	tmp = normalize(tmp);

	vec3 chromeupvec = cross(tmp, r_vright);
	chromeupvec = normalize(chromeupvec);

	vec3 chromerightvec = cross(tmp, chromeupvec);
	chromerightvec = normalize(chromerightvec);

	vec3 chromeup = vec3(
		chromeupvec.x * normbone_matrix[0][0] + chromeupvec.y * normbone_matrix[1][0] + chromeupvec.z * normbone_matrix[2][0],
		chromeupvec.x * normbone_matrix[0][1] + chromeupvec.y * normbone_matrix[1][1] + chromeupvec.z * normbone_matrix[2][1],
		chromeupvec.x * normbone_matrix[0][2] + chromeupvec.y * normbone_matrix[1][2] + chromeupvec.z * normbone_matrix[2][2]
	);

	vec3 chromeright = vec3(
		chromerightvec.x * normbone_matrix[0][0] + chromerightvec.y * normbone_matrix[1][0] + chromerightvec.z * normbone_matrix[2][0],
		chromerightvec.x * normbone_matrix[0][1] + chromerightvec.y * normbone_matrix[1][1] + chromerightvec.z * normbone_matrix[2][1],
		chromerightvec.x * normbone_matrix[0][2] + chromerightvec.y * normbone_matrix[1][2] + chromerightvec.z * normbone_matrix[2][2]
	);
	
	vec2 texcoord = vec2(
		(dot(norm, chromeright) + 1.0),
		(dot(norm, chromeup) + 1.0)
	);

	if(r_scale > 0.0)
	{
		texcoord.x *= gl_MultiTexCoord0.x;
		texcoord.y *= gl_MultiTexCoord0.y;
	}
	else
	{
		texcoord.x *= 1024.0 / 2048.0;
		texcoord.y *= 1024.0 / 2048.0;
	}

	gl_TexCoord[0] = vec4(texcoord.x, texcoord.y, 0.0, 0.0);

#else

	gl_TexCoord[0] = gl_MultiTexCoord0;

#endif

	gl_Position = gl_ModelViewProjectionMatrix * vec4(outvert, 1.0);
}