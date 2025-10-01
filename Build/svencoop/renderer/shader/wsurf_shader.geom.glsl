#version 430

#extension GL_EXT_texture_array : require

#include "common.h"

// Input from vertex shader
layout(triangles) in;

// Output configuration
#ifdef WSURF_MULTIVIEW_ENABLED
	// Maximum vertices: 3 per triangle * 6 views (for cubemap shadow)
	layout(triangle_strip, max_vertices = 18) out;
#else
	layout(triangle_strip, max_vertices = 3) out;
#endif

// Input from vertex shader
// Note: For geometry shader, the outermost dimension must be explicitly sized for AMD compatibility
in vec3 v_worldpos[3];
in vec3 v_normal[3];
in vec3 v_tangent[3];
in vec3 v_bitangent[3];
in vec2 v_diffusetexcoord[3];
in vec3 v_lightmaptexcoord[3];
in vec2 v_detailtexcoord[3];
in vec2 v_normaltexcoord[3];
in vec2 v_parallaxtexcoord[3];
in vec2 v_speculartexcoord[3];
in vec4 v_shadowcoord[3][3];  // [vertex_index][shadow_cascade_index]
in vec4 v_projpos[3];

#if defined(SKYBOX_ENABLED)
	
#elif defined(DECAL_ENABLED)
	flat in uvec4 v_styles[3];
#else
	flat in uvec4 v_styles[3];
#endif

// Output to fragment shader
out vec3 g_worldpos;
out vec3 g_normal;
out vec3 g_tangent;
out vec3 g_bitangent;
out vec2 g_diffusetexcoord;
out vec3 g_lightmaptexcoord;
out vec2 g_detailtexcoord;
out vec2 g_normaltexcoord;
out vec2 g_parallaxtexcoord;
out vec2 g_speculartexcoord;
out vec4 g_shadowcoord[3];
out vec4 g_projpos;

#if defined(SKYBOX_ENABLED)
	
#elif defined(DECAL_ENABLED)
	flat out uvec4 g_styles;
#else
	flat out uvec4 g_styles;
#endif

void main()
{
#ifdef WSURF_MULTIVIEW_ENABLED
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
			g_tangent = v_tangent[i];
			g_bitangent = v_bitangent[i];
			g_diffusetexcoord = v_diffusetexcoord[i];
			g_lightmaptexcoord = v_lightmaptexcoord[i];
			g_detailtexcoord = v_detailtexcoord[i];
			g_normaltexcoord = v_normaltexcoord[i];
			g_parallaxtexcoord = v_parallaxtexcoord[i];
			g_speculartexcoord = v_speculartexcoord[i];
			g_shadowcoord[0] = v_shadowcoord[i][0];
			g_shadowcoord[1] = v_shadowcoord[i][1];
			g_shadowcoord[2] = v_shadowcoord[i][2];
			g_projpos = gl_Position;
			
			#if !defined(SKYBOX_ENABLED)
				g_styles = v_styles[i];
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
		g_tangent = v_tangent[i];
		g_bitangent = v_bitangent[i];
		g_diffusetexcoord = v_diffusetexcoord[i];
		g_lightmaptexcoord = v_lightmaptexcoord[i];
		g_detailtexcoord = v_detailtexcoord[i];
		g_normaltexcoord = v_normaltexcoord[i];
		g_parallaxtexcoord = v_parallaxtexcoord[i];
		g_speculartexcoord = v_speculartexcoord[i];
		g_shadowcoord[0] = v_shadowcoord[i][0];
		g_shadowcoord[1] = v_shadowcoord[i][1];
		g_shadowcoord[2] = v_shadowcoord[i][2];
		g_projpos = v_projpos[i];
		
		#if !defined(SKYBOX_ENABLED)
			g_styles = v_styles[i];
		#endif
		
		EmitVertex();
	}
	EndPrimitive();
#endif
}

