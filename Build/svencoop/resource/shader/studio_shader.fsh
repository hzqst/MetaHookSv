uniform sampler2D diffuseTex;
uniform float r_scale;

varying vec4 worldpos;
varying vec4 normal;
varying vec4 color;

void main(void)
{
	vec4 diffuseColor = texture2D(diffuseTex, gl_TexCoord[0].xy);

#ifdef GBUFFER_ENABLED
	#ifdef STUDIO_CHROME
		if(r_scale > 0.0)
		{
			gl_FragData[0] = diffuseColor * color;
		}
		else
		{
			gl_FragData[0] = diffuseColor;
			gl_FragData[1] = color;
			gl_FragData[2] = worldpos;
			gl_FragData[3] = normal;
		}
	#else
		gl_FragData[0] = diffuseColor;
		gl_FragData[1] = color;
		gl_FragData[2] = worldpos;
		gl_FragData[3] = normal;
	#endif
#else
	gl_FragColor = diffuseColor * color;
#endif
}