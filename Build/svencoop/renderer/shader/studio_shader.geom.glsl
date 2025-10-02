#version 430

#include "common.h"

// Input from vertex shader
layout(triangles) in;

// Output configuration
#ifdef STUDIO_MULTIVIEW_ENABLED
	// Maximum vertices: 3 per triangle * 6 views (for cubemap shadow)
	layout(triangle_strip, max_vertices = 18) out;
#else
	layout(triangle_strip, max_vertices = 3) out;
#endif

// Input from vertex shader
// Note: For AMD compatibility, explicitly size all array dimensions
in vec3 v_worldpos[3];
in vec3 v_normal[3];
in vec2 v_texcoord[3];
in vec4 v_projpos[3];
flat in uint v_packedbone[3];
in vec3 v_tangent[3];
in vec3 v_bitangent[3];
in vec3 v_smoothnormal[3];

#if defined(STUDIO_NF_CELSHADE_FACE)
	in vec3 v_headfwd[3];
	in vec3 v_headup[3];
	in vec3 v_headorigin[3];
#endif

// Output to fragment shader
out vec3 g_worldpos;
out vec3 g_normal;
out vec2 g_texcoord;
out vec4 g_projpos;
flat out uint g_packedbone;
out vec3 g_tangent;
out vec3 g_bitangent;
out vec3 g_smoothnormal;

#if defined(STUDIO_NF_CELSHADE_FACE)
	out vec3 g_headfwd;
	out vec3 g_headup;
	out vec3 g_headorigin;
#endif

void main()
{
#ifdef STUDIO_MULTIVIEW_ENABLED
	// Render to multiple views (for cubemap shadow or CSM)
	int numViews = CameraUBO.numViews;
	
	for (int viewIdx = 0; viewIdx < numViews; ++viewIdx)
	{
		// Set the layer for texture array rendering
		gl_Layer = viewIdx;
		
		// Emit all 3 vertices for this triangle
		for (int i = 0; i < 3; ++i)
		{
			// Transform position to clip space using the specific view's matrices
			vec4 worldPos = vec4(v_worldpos[i], 1.0);
			gl_Position = GetCameraProjMatrix(viewIdx) * GetCameraWorldMatrix(viewIdx) * worldPos;
			
			// Pass through all attributes
			g_worldpos = v_worldpos[i];
			g_normal = v_normal[i];
			g_texcoord = v_texcoord[i];
			g_projpos = gl_Position;
			g_packedbone = v_packedbone[i];
			g_tangent = v_tangent[i];
			g_bitangent = v_bitangent[i];
			g_smoothnormal = v_smoothnormal[i];
			
			#if defined(STUDIO_NF_CELSHADE_FACE)
				g_headfwd = v_headfwd[i];
				g_headup = v_headup[i];
				g_headorigin = v_headorigin[i];
			#endif
			
			EmitVertex();
		}
		EndPrimitive();
	}
#else
	// Standard single-view rendering - pass through
	for (int i = 0; i < 3; ++i)
	{
		gl_Position = gl_in[i].gl_Position;
		
		g_worldpos = v_worldpos[i];
		g_normal = v_normal[i];
		g_texcoord = v_texcoord[i];
		g_projpos = v_projpos[i];
		g_packedbone = v_packedbone[i];
		g_tangent = v_tangent[i];
		g_bitangent = v_bitangent[i];
		g_smoothnormal = v_smoothnormal[i];
		
		#if defined(STUDIO_NF_CELSHADE_FACE)
			g_headfwd = v_headfwd[i];
			g_headup = v_headup[i];
			g_headorigin = v_headorigin[i];
		#endif
		
		EmitVertex();
	}
	EndPrimitive();
#endif
}

