uniform sampler2D diffuseTex;

varying vec4 worldpos;
varying vec4 normal;
varying vec4 color;

void main(void)
{
	vec4 diffuseColor = texture2D(diffuseTex, gl_TexCoord[0].xy);

#ifdef GBUFFER_ENABLED
	gl_FragData[0] = diffuseColor;
    gl_FragData[1] = color;
    gl_FragData[2] = worldpos;
    gl_FragData[3] = normal;
#else
	gl_FragColor = diffuseColor * color;
#endif
}