//Fragment Shader by hzqst

uniform sampler2D base;
uniform vec2 alpha_range;
uniform vec2 offset;
void main()
{
	vec4 vBaseColor = texture2D(base, gl_TexCoord[0].xy);
	vBaseColor += texture2D(base, gl_TexCoord[0].xy + vec2(offset.x * 1.0, 0.0));
	vBaseColor += texture2D(base, gl_TexCoord[0].xy + vec2(offset.x * -1.0, 0.0));
	vBaseColor += texture2D(base, gl_TexCoord[0].xy + vec2(0.0, offset.y * 1.0));
	vBaseColor += texture2D(base, gl_TexCoord[0].xy + vec2(0.0, offset.y * 1.0));
	vBaseColor /= 5.0;

	if(vBaseColor.a < alpha_range.x)
		vBaseColor.a = 1.0 - (alpha_range.x - vBaseColor.a)/alpha_range.x;
	else if(vBaseColor.a >= alpha_range.x && vBaseColor.a <= alpha_range.y)
		vBaseColor.a = 1.0;
	else
		vBaseColor.a = 1.0 - (vBaseColor.a - alpha_range.y)/(1.0 - alpha_range.y);
	gl_FragColor = vBaseColor;
}