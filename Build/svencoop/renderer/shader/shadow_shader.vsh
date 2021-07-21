uniform mat4 entitymatrix;

varying vec4 shadowcoord_high;
varying vec4 shadowcoord_medium;
varying vec4 shadowcoord_low;

varying vec4 scene_worldpos;

void main()
{
  	shadowcoord_high = gl_TextureMatrix[0] * gl_ModelViewMatrix * gl_Vertex;
  	shadowcoord_medium = gl_TextureMatrix[1] * gl_ModelViewMatrix * gl_Vertex;
  	shadowcoord_low = gl_TextureMatrix[2] * gl_ModelViewMatrix * gl_Vertex;

	scene_worldpos = entitymatrix * gl_Vertex;

	gl_Position = ftransform();
}