#version 120

uniform sampler2D tex;

const vec3 LUMINANCE_WEIGHTS = vec3(0.27, 0.67, 0.06);

void main() 
{
    float sample = 0.0;

	sample += log( dot( texture2D( tex,  gl_TexCoord[0].xy ).xyz,  LUMINANCE_WEIGHTS ) + 0.0001 );
	sample += log( dot( texture2D( tex,  gl_TexCoord[0].zw ).xyz,  LUMINANCE_WEIGHTS ) + 0.0001 );
	sample += log( dot( texture2D( tex,  gl_TexCoord[1].xy ).xyz,  LUMINANCE_WEIGHTS ) + 0.0001 );
	sample += log( dot( texture2D( tex,  gl_TexCoord[1].zw ).xyz,  LUMINANCE_WEIGHTS ) + 0.0001 );

	sample *= 1.0 / 4.0;

	gl_FragColor = vec4(sample, sample, sample, 1.0);
}