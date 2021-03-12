uniform sampler2D tex;
varying float lum;

void main() 
{
	vec4 vColor = texture2D(tex, gl_TexCoord[0].xy);
	float fLumValue = dot(vec3(0.27, 0.67, 0.06), vColor.rgb);
	
	gl_FragColor = max(vColor *(fLumValue - lum) * 5.0, vec4(0.0, 0.0, 0.0, 1.0));

	//vColor.rgb *= 3.0;
	//vColor.rgb -= 2.0;
	//vColor.rgb = max(vColor.rgb, vec3(0.0, 0.0, 0.0));

	//gl_FragColor = vColor;
}