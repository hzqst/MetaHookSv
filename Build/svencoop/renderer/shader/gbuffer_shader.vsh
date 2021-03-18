#ifdef ROTATE_ENABLED
uniform mat4 entitymatrix;
#endif

varying vec4 worldpos;
varying vec4 normal;
varying vec4 color;

void main()
{
  worldpos = gl_Vertex;
  normal = vec4(gl_Normal, 0.0);

#ifdef DIFFUSE_ENABLED
  
  gl_TexCoord[0] = gl_MultiTexCoord0;

#endif

#ifdef LIGHTMAP_ENABLED
  gl_TexCoord[1] = gl_MultiTexCoord1;
#endif

#ifdef DETAILTEXTURE_ENABLED
  gl_TexCoord[2] = gl_MultiTexCoord2;
#endif

  color = gl_Color;

  gl_Position = ftransform();
}