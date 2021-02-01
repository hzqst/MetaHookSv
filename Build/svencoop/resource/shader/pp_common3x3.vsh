uniform vec2 texelsize;

void main(void)
{
    gl_TexCoord[0] = gl_MultiTexCoord0;
    gl_TexCoord[1] = gl_MultiTexCoord0 + vec2(texelsize.x, 0.0);
    gl_TexCoord[2] = gl_MultiTexCoord0 + vec2(texelsize.x, texelsize.y);
    gl_TexCoord[3] = gl_MultiTexCoord0 + vec2(0.0, texelsize.y);
    gl_TexCoord[4] = gl_MultiTexCoord0 + vec2(-texelsize.x, 0.0);
    gl_TexCoord[5] = gl_MultiTexCoord0 + vec2(-texelsize.x, -texelsize.y);
    gl_TexCoord[6] = gl_MultiTexCoord0 + vec2(0.0, -texelsize.y);
	gl_TexCoord[7] = gl_MultiTexCoord0 + vec2(texelsize.x, -texelsize.y);
	gl_TexCoord[8] = gl_MultiTexCoord0 + vec2(-texelsize.x, texelsize.y);
	gl_Position = ftransform();
}