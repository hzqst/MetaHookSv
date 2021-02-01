//fragment shader
uniform sampler2D basemap;
uniform sampler2D normalmap;
uniform vec4 ambient, diffuse, specular;
uniform float shiness;
varying vec3 lightvec;
varying vec3 halfvec;

void main(void)
{
	vec3 vlightvec = normalize(lightvec);
	vec3 vhalfvec =  normalize(halfvec);
	vec4 baseCol = texture2D(basemap, gl_TexCoord[0].xy); 
	vec3 tbnnorm = texture2D(normalmap, gl_TexCoord[0].xy).xyz;
   
	tbnnorm = normalize(tbnnorm * 2.0 - vec3(1.0));

	float diffusefract = max(0.0, dot(vlightvec, tbnnorm));
	float specularfract = max(0.0, dot(vhalfvec, tbnnorm));
	if(specularfract > 0.0)
 	{
		specularfract = pow(specularfract, shiness);
	}

	gl_FragColor = vec4(ambient.xyz * baseCol.xyz + 
	gl_Color.xyz * diffuse.xyz * baseCol.xyz +
	specular.xyz * specularfract, 1.0);
}