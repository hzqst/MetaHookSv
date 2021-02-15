varying vec4 worldpos;
varying vec4 normal;

void main()
{
  worldpos = vec4(gl_Vertex.xyz / gl_Vertex.w, 1.0);
  //worldpos.x *= 1.0 / 1024.0;
  //worldpos.y *= 1.0 / 1024.0;
  //worldpos.z *= 1.0 / 1024.0;

  normal = vec4(normalize(gl_Normal), 1.0);

  gl_TexCoord[0] = gl_MultiTexCoord0;
  
#ifdef LIGHTMAP_ENABLED
  gl_TexCoord[1] = gl_MultiTexCoord1;
#endif

#ifdef DETAILTEXTURE_ENABLED
  gl_TexCoord[2] = gl_MultiTexCoord2;
#endif

  gl_FrontColor = gl_Color;
  gl_Position = ftransform();
}