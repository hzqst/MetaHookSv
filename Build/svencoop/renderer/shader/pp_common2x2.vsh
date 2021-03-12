uniform vec2 texelsize;

void main(void)
{
    gl_TexCoord[0].xy = gl_MultiTexCoord0.xy;
	gl_TexCoord[0].zw = gl_MultiTexCoord0.xy + vec2(texelsize.x, 0.0);
    gl_TexCoord[1].xy = gl_MultiTexCoord0.xy + vec2(texelsize.x, texelsize.y);
    gl_TexCoord[1].zw = gl_MultiTexCoord0.xy + vec2(0.0, texelsize.y);
	gl_Position = ftransform();
}