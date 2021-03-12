uniform sampler2D tex;
    
void main() 
{ 
    vec4 sample = vec4(0.0, 0.0, 0.0, 0.0);

	sample += texture2D( tex,  gl_TexCoord[0].xy );
	sample += texture2D( tex,  gl_TexCoord[0].zw );
	sample += texture2D( tex,  gl_TexCoord[1].xy );
	sample += texture2D( tex,  gl_TexCoord[1].zw );

	sample *= 1.0 / 4.0;
	
	gl_FragColor = sample;
}