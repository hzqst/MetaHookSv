#include "gl_local.h"
#include <sstream>

//HDR
int last_luminance = 0;

#define MAX_GAUSSIAN_SAMPLES 16

#define LUMPASS_DOWN 0
#define LUMPASS_LOG 1
#define LUMPASS_EXP 2

//HBAO
#define AO_RANDOMTEX_SIZE 4
static const int  NUM_MRT = 8;
static const int  HBAO_RANDOM_SIZE = AO_RANDOMTEX_SIZE;
static const int  HBAO_RANDOM_ELEMENTS = HBAO_RANDOM_SIZE * HBAO_RANDOM_SIZE;
static const int  MAX_SAMPLES = 16;

GLuint hbao_random = 0;
GLuint hbao_randomview[MAX_SAMPLES] = { 0 };
vec4_t m_hbaoRandom[HBAO_RANDOM_ELEMENTS * MAX_SAMPLES] = { 0 };

//FXAA
SHADER_DEFINE(pp_fxaa);

//HDR
SHADER_DEFINE(pp_downsample);
SHADER_DEFINE(pp_downsample2x2);
SHADER_DEFINE(pp_lumindown);
SHADER_DEFINE(pp_luminlog);
SHADER_DEFINE(pp_luminexp);
SHADER_DEFINE(pp_luminadapt);
SHADER_DEFINE(pp_brightpass);
SHADER_DEFINE(pp_gaussianblurv);
SHADER_DEFINE(pp_gaussianblurh);
SHADER_DEFINE(pp_tonemap);

//HBAO
SHADER_DEFINE(depth_linearize);
SHADER_DEFINE(hbao_calc_blur);
SHADER_DEFINE(hbao_calc_blur_fog);
SHADER_DEFINE(hbao_blur);
SHADER_DEFINE(hbao_blur2);

SHADER_DEFINE(oitbuffer_clear);
SHADER_DEFINE(blit_oitblend);

SHADER_DEFINE(gamma_correction);
SHADER_DEFINE(gamma_uncorrection);
SHADER_DEFINE(copy_color);
SHADER_DEFINE(copy_color_halo_add);

SHADER_DEFINE(under_water_effect);

std::unordered_map<program_state_t, drawtexturedrect_program_t> g_DrawTexturedRectProgramTable;
std::unordered_map<program_state_t, drawfilledrect_program_t> g_DrawFilledRectProgramTable;

IPMBRingBuffer* g_TexturedRectVertexBuffer{};
IPMBRingBuffer* g_FilledRectVertexBuffer{};
IPMBRingBuffer* g_RectInstanceBuffer{};
IPMBRingBuffer* g_RectIndexBuffer{};

cvar_t *r_hdr = NULL;

MapConVar *r_hdr_blurwidth = NULL;
MapConVar *r_hdr_exposure = NULL;
MapConVar *r_hdr_darkness = NULL;
MapConVar *r_hdr_adaptation = NULL;

cvar_t *r_fxaa = NULL;

cvar_t* r_under_water_effect = NULL;
cvar_t* r_under_water_effect_wave_amount = NULL;
cvar_t* r_under_water_effect_wave_speed = NULL;
cvar_t* r_under_water_effect_wave_size = NULL;

cvar_t *r_ssao = NULL;

MapConVar *r_ssao_radius = NULL;
MapConVar *r_ssao_intensity = NULL;
MapConVar *r_ssao_bias = NULL;
MapConVar *r_ssao_blur_sharpness = NULL;

void R_UseDrawTexturedRectProgram(program_state_t state, drawtexturedrect_program_t* progOutput)
{
	drawtexturedrect_program_t prog = { 0 };

	auto itor = g_DrawTexturedRectProgramTable.find(state);
	if (itor == g_DrawTexturedRectProgramTable.end())
	{
		std::stringstream defs;

		if (state & DRAW_TEXTURED_RECT_ALPHA_TEST_ENABLED)
		{
			defs << "#define ALPHA_TEST_ENABLED\n";
		}

		if (state & DRAW_TEXTURED_RECT_MASK_TEXTURE_ENABLED)
		{
			defs << "#define MASK_TEXTURE_ENABLED\n";
		}

		auto def = defs.str();

		prog.program = GL_CompileShaderFile(
			"renderer\\shader\\drawtexturedrect_shader.vert.glsl", 
			"renderer\\shader\\drawtexturedrect_shader.frag.glsl", 
			def.c_str(), def.c_str());

		if (prog.program)
		{

		}

		g_DrawTexturedRectProgramTable[state] = prog;
	}
	else
	{
		prog = itor->second;
	}

	if (prog.program)
	{
		GL_UseProgram(prog.program);

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		Sys_Error("R_UseDrawTexturedRectProgram: Failed to load program!");
	}
}

const program_state_mapping_t s_DrawTexturedRectProgramStateName[] = {
	{ DRAW_TEXTURED_RECT_ALPHA_BLEND_ENABLED			 ,"DRAW_TEXTURED_RECT_ALPHA_BLEND_ENABLED"				},
	{ DRAW_TEXTURED_RECT_ADDITIVE_BLEND_ENABLED			 ,"DRAW_TEXTURED_RECT_ADDITIVE_BLEND_ENABLED"			},
	{ DRAW_TEXTURED_RECT_ALPHA_BASED_ADDITIVE_ENABLED	 ,"DRAW_TEXTURED_RECT_ALPHA_BASED_ADDITIVE_ENABLED"		},
	{ DRAW_TEXTURED_RECT_SCISSOR_ENABLED				 ,"DRAW_TEXTURED_RECT_SCISSOR_ENABLED"					},
	{ DRAW_TEXTURED_RECT_ALPHA_TEST_ENABLED				 ,"DRAW_TEXTURED_RECT_ALPHA_TEST_ENABLED"				},
	{ DRAW_TEXTURED_RECT_MASK_TEXTURE_ENABLED			 ,"DRAW_TEXTURED_RECT_MASK_TEXTURE_ENABLED"				},
};

void R_SaveDrawTexturedRectProgramStates(void)
{
	std::vector<program_state_t> states;

	for (auto& p : g_DrawTexturedRectProgramTable)
	{
		states.emplace_back(p.first);
	}

	R_SaveProgramStatesCaches("renderer/shader/drawtexturedrect_cache.txt", states, s_DrawTexturedRectProgramStateName, _ARRAYSIZE(s_DrawTexturedRectProgramStateName));
}

void R_LoadDrawTexturedRectProgramStates(void)
{
	R_LoadProgramStateCaches("renderer/shader/drawtexturedrect_cache.txt", s_DrawTexturedRectProgramStateName, _ARRAYSIZE(s_DrawTexturedRectProgramStateName), [](program_state_t state) {

		R_UseDrawTexturedRectProgram(state, NULL);

	});
}

void R_UseDrawFilledRectProgram(program_state_t state, drawfilledrect_program_t* progOutput)
{
	drawfilledrect_program_t prog = { 0 };

	auto itor = g_DrawFilledRectProgramTable.find(state);
	if (itor == g_DrawFilledRectProgramTable.end())
	{
		std::stringstream defs;

		auto def = defs.str();

		prog.program = GL_CompileShaderFile(
			"renderer\\shader\\drawfilledrect_shader.vert.glsl", 
			"renderer\\shader\\drawfilledrect_shader.frag.glsl", 
			def.c_str(), def.c_str());

		if (prog.program)
		{

		}

		g_DrawFilledRectProgramTable[state] = prog;
	}
	else
	{
		prog = itor->second;
	}

	if (prog.program)
	{
		GL_UseProgram(prog.program);

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		Sys_Error("R_UseDrawFilledRectProgram: Failed to load program!");
	}
}

const program_state_mapping_t s_DrawFilledRectProgramStateName[] = {
	{ DRAW_FILLED_RECT_ALPHA_BLEND_ENABLED				,"DRAW_FILLED_RECT_ALPHA_BLEND_ENABLED"				},
	{ DRAW_FILLED_RECT_ADDITIVE_BLEND_ENABLED			,"DRAW_FILLED_RECT_ADDITIVE_BLEND_ENABLED"			},
	{ DRAW_FILLED_RECT_ALPHA_BASED_ADDITIVE_ENABLED		,"DRAW_FILLED_RECT_ALPHA_BASED_ADDITIVE_ENABLED"	},
	{ DRAW_FILLED_RECT_SCISSOR_ENABLED					,"DRAW_FILLED_RECT_SCISSOR_ENABLED"					},
	{ DRAW_FILLED_RECT_ZERO_SRC_ALPHA_BLEND_ENABLED		,"DRAW_FILLED_RECT_ZERO_SRC_ALPHA_BLEND_ENABLED"	},
	{ DRAW_FILLED_RECT_LINE_ENABLED						,"DRAW_FILLED_RECT_LINE_ENABLED"	},
};

void R_SaveDrawFilledRectProgramStates(void)
{
	std::vector<program_state_t> states;

	for (auto& p : g_DrawFilledRectProgramTable)
	{
		states.emplace_back(p.first);
	}

	R_SaveProgramStatesCaches("renderer/shader/drawfilledrect_cache.txt", states, s_DrawFilledRectProgramStateName, _ARRAYSIZE(s_DrawFilledRectProgramStateName));
}

void R_LoadDrawFilledRectProgramStates(void)
{
	R_LoadProgramStateCaches("renderer/shader/drawfilledrect_cache.txt", s_DrawFilledRectProgramStateName, _ARRAYSIZE(s_DrawFilledRectProgramStateName), [](program_state_t state) {

		R_UseDrawFilledRectProgram(state, NULL);

		});
}

void R_InitHUD(void)
{
	float numDir = 8; // keep in sync to glsl

	signed short hbaoRandomShort[HBAO_RANDOM_ELEMENTS * MAX_SAMPLES * 4];

	for (int i = 0; i < HBAO_RANDOM_ELEMENTS * MAX_SAMPLES; i++)
	{
		float Rand1 = gEngfuncs.pfnRandomFloat(0, 1);
		float Rand2 = gEngfuncs.pfnRandomFloat(0, 1);

		// Use random rotation angles in [0,2PI/NUM_DIRECTIONS)
		float Angle = 2.f * M_PI * Rand1 / numDir;
		m_hbaoRandom[i][0] = cosf(Angle);
		m_hbaoRandom[i][1] = sinf(Angle);
		m_hbaoRandom[i][2] = Rand2;
		m_hbaoRandom[i][3] = 0;
#define SCALE ((1<<15))
		hbaoRandomShort[i * 4 + 0] = (signed short)(SCALE*m_hbaoRandom[i][0]);
		hbaoRandomShort[i * 4 + 1] = (signed short)(SCALE*m_hbaoRandom[i][1]);
		hbaoRandomShort[i * 4 + 2] = (signed short)(SCALE*m_hbaoRandom[i][2]);
		hbaoRandomShort[i * 4 + 3] = (signed short)(SCALE*m_hbaoRandom[i][3]);
#undef SCALE
	}

	hbao_random = GL_GenTexture();
	glBindTexture(GL_TEXTURE_2D_ARRAY, hbao_random);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA16_SNORM, HBAO_RANDOM_SIZE, HBAO_RANDOM_SIZE, MAX_SAMPLES);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, HBAO_RANDOM_SIZE, HBAO_RANDOM_SIZE, MAX_SAMPLES, GL_RGBA, GL_SHORT, hbaoRandomShort);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	for (int i = 0; i < MAX_SAMPLES; i++)
	{
		hbao_randomview[i] = GL_GenTexture();
		glTextureView(hbao_randomview[i], GL_TEXTURE_2D, hbao_random, GL_RGBA16_SNORM, 0, 1, i, 1);
		glBindTexture(GL_TEXTURE_2D, hbao_randomview[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	//FXAA Pass
	pp_fxaa.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\pp_fxaa.frag.glsl");

	//DownSample Pass
	pp_downsample.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\down_sample.frag.glsl");
	
	//2x2 Downsample Pass
	pp_downsample2x2.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\down_sample.frag.glsl", "", "#define DOWNSAMPLE_2X2\n");

	//Luminance Downsample Pass
	pp_lumindown.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\hdr_lumpass.frag.glsl", "", "");

	//Log Luminance Downsample Pass
	pp_luminlog.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\hdr_lumpass.frag.glsl", "", "#define LUMPASS_LOG\n");

	//Exp Luminance Downsample Pass
	pp_luminexp.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\hdr_lumpass.frag.glsl", "", "#define LUMPASS_EXP\n");

	//Luminance Adaptation Downsample Pass
	pp_luminadapt.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\hdr_adaption.frag.glsl", "", "");

	//Bright Pass
	pp_brightpass.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\hdr_brightpass.frag.glsl", "", "");

	//Tone mapping
	pp_tonemap.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\hdr_tonemap.frag.glsl", "", "");

	//SSAO
	depth_linearize.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\depthlinearize.frag.glsl");

	hbao_calc_blur.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\hbao.frag.glsl");

	if (hbao_calc_blur.program)
	{
		SHADER_UNIFORM(hbao_calc_blur, texLinearDepth, "texLinearDepth");
		SHADER_UNIFORM(hbao_calc_blur, texRandom, "texRandom");
		SHADER_UNIFORM(hbao_calc_blur, control_RadiusToScreen, "control_RadiusToScreen");
		SHADER_UNIFORM(hbao_calc_blur, control_projOrtho, "control_projOrtho");
		SHADER_UNIFORM(hbao_calc_blur, control_projInfo, "control_projInfo");
		SHADER_UNIFORM(hbao_calc_blur, control_PowExponent, "control_PowExponent");
		SHADER_UNIFORM(hbao_calc_blur, control_InvQuarterResolution, "control_InvQuarterResolution");
		SHADER_UNIFORM(hbao_calc_blur, control_AOMultiplier, "control_AOMultiplier");
		SHADER_UNIFORM(hbao_calc_blur, control_InvFullResolution, "control_InvFullResolution");
		SHADER_UNIFORM(hbao_calc_blur, control_NDotVBias, "control_NDotVBias");
		SHADER_UNIFORM(hbao_calc_blur, control_NegInvR2, "control_NegInvR2");
	}

	//OIT Blend
	if (g_bUseOITBlend)
	{
		oitbuffer_clear.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\oitbuffer_clear.frag.glsl");

		blit_oitblend.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\blit_oitblend.frag.glsl");
	}

	gamma_correction.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\gamma_correction.frag.glsl");

	gamma_uncorrection.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\gamma_uncorrection.frag.glsl");

	copy_color.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\copy_color.frag.glsl");

	copy_color_halo_add.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\copy_color.frag.glsl", "#define HALO_ADD_ENABLED\n", "#define HALO_ADD_ENABLED\n");

	under_water_effect.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\under_water_effect.frag.glsl");

	hbao_calc_blur_fog.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\hbao.frag.glsl", "#define LINEAR_FOG_ENABLED\n", "#define LINEAR_FOG_ENABLED\n");

	if (hbao_calc_blur_fog.program)
	{
		SHADER_UNIFORM(hbao_calc_blur_fog, texLinearDepth, "texLinearDepth");
		SHADER_UNIFORM(hbao_calc_blur_fog, texRandom, "texRandom");
		SHADER_UNIFORM(hbao_calc_blur_fog, control_RadiusToScreen, "control_RadiusToScreen");
		SHADER_UNIFORM(hbao_calc_blur_fog, control_projOrtho, "control_projOrtho");
		SHADER_UNIFORM(hbao_calc_blur_fog, control_projInfo, "control_projInfo");
		SHADER_UNIFORM(hbao_calc_blur_fog, control_PowExponent, "control_PowExponent");
		SHADER_UNIFORM(hbao_calc_blur_fog, control_InvQuarterResolution, "control_InvQuarterResolution");
		SHADER_UNIFORM(hbao_calc_blur_fog, control_AOMultiplier, "control_AOMultiplier");
		SHADER_UNIFORM(hbao_calc_blur_fog, control_InvFullResolution, "control_InvFullResolution");
		SHADER_UNIFORM(hbao_calc_blur_fog, control_NDotVBias, "control_NDotVBias");
		SHADER_UNIFORM(hbao_calc_blur_fog, control_NegInvR2, "control_NegInvR2");
		SHADER_UNIFORM(hbao_calc_blur_fog, control_Fog, "control_Fog");
	}

	hbao_blur.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\hbao_blur.frag.glsl");

	hbao_blur2.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\hbao_blur.frag.glsl", "#define AO_BLUR_PRESENT\n", "#define AO_BLUR_PRESENT\n");

	pp_gaussianblurh.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\gaussian_blur_16x.frag.glsl", "", "#define BLUR_HORIZONAL\n");
	
	pp_gaussianblurv.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\gaussian_blur_16x.frag.glsl", "", "#define BLUR_VERTICAL\n");

	r_hdr = gEngfuncs.pfnRegisterVariable("r_hdr", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_hdr_blurwidth = R_RegisterMapCvar("r_hdr_blurwidth", "0.075", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_hdr_exposure = R_RegisterMapCvar("r_hdr_exposure", "1.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_hdr_darkness = R_RegisterMapCvar("r_hdr_darkness", "1.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_hdr_adaptation = R_RegisterMapCvar("r_hdr_adaptation", "50", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_fxaa = gEngfuncs.pfnRegisterVariable("r_fxaa", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_under_water_effect = gEngfuncs.pfnRegisterVariable("r_under_water_effect", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_under_water_effect_wave_amount = gEngfuncs.pfnRegisterVariable("r_under_water_effect_wave_amount", "10", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_under_water_effect_wave_speed = gEngfuncs.pfnRegisterVariable("r_under_water_effect_wave_speed", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_under_water_effect_wave_size = gEngfuncs.pfnRegisterVariable("r_under_water_effect_wave_size", "0.01", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_ssao = gEngfuncs.pfnRegisterVariable("r_ssao", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_ssao_radius = R_RegisterMapCvar("r_ssao_radius", "30.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssao_intensity = R_RegisterMapCvar("r_ssao_intensity", "3.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssao_bias = R_RegisterMapCvar("r_ssao_bias", "0.2", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssao_blur_sharpness = R_RegisterMapCvar("r_ssao_blur_sharpness", "1.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	last_luminance = 0;


}

class CDrawRectCommand
{
public:
	GLuint hVAO{};
};

CDrawRectCommand g_DrawTexturedRectCommand;

void R_DrawTexturedRect(int gltexturenum, const texturedrectvertex_t *verticeBuffer, size_t verticeCount, const uint32_t *indices, size_t indicesCount, uint64_t programState, const char * debugMetadata)
{
	if (!g_DrawTexturedRectCommand.hVAO)
	{
		g_DrawTexturedRectCommand.hVAO = GL_GenVAO();

		if (!g_TexturedRectVertexBuffer)
		{
			g_TexturedRectVertexBuffer = GL_CreatePMBRingBuffer("TexturedRectVertexBuffer", 16 * 1024 * 1024, GL_ARRAY_BUFFER);
		}

		if (!g_RectInstanceBuffer)
		{
			g_RectInstanceBuffer = GL_CreatePMBRingBuffer("RectInstanceBuffer", 8 * 1024 * 1024, GL_ARRAY_BUFFER);
		}

		if (!g_RectIndexBuffer)
		{
			g_RectIndexBuffer = GL_CreatePMBRingBuffer("RectIndexBuffer", 1 * 1024 * 1024, GL_ELEMENT_ARRAY_BUFFER);
		}

		GL_BindStatesForVAO(
			g_DrawTexturedRectCommand.hVAO,
			[&]() {
				glBindBuffer(GL_ARRAY_BUFFER, g_TexturedRectVertexBuffer->GetGLBufferObject());

				glVertexAttribPointer(TEXTUREDRECT_VA_POSITION, 2, GL_FLOAT, false, sizeof(texturedrectvertex_t), OFFSET(texturedrectvertex_t, pos));
				glEnableVertexAttribArray(TEXTUREDRECT_VA_POSITION);

				glVertexAttribPointer(TEXTUREDRECT_VA_TEXCOORD, 2, GL_FLOAT, false, sizeof(texturedrectvertex_t), OFFSET(texturedrectvertex_t, texcoord));
				glEnableVertexAttribArray(TEXTUREDRECT_VA_TEXCOORD);

				glVertexAttribPointer(TEXTUREDRECT_VA_COLOR, 4, GL_FLOAT, false, sizeof(texturedrectvertex_t), OFFSET(texturedrectvertex_t, col));
				glEnableVertexAttribArray(TEXTUREDRECT_VA_COLOR);

				glBindBuffer(GL_ARRAY_BUFFER, g_RectInstanceBuffer->GetGLBufferObject());

				glVertexAttribPointer(TEXTUREDRECT_VA_MATRIX0, 4, GL_FLOAT, false, sizeof(rect_instance_data_t), OFFSET(rect_instance_data_t, matrix[0]));
				glVertexAttribDivisor(TEXTUREDRECT_VA_MATRIX0, 1);
				glEnableVertexAttribArray(TEXTUREDRECT_VA_MATRIX0);

				glVertexAttribPointer(TEXTUREDRECT_VA_MATRIX1, 4, GL_FLOAT, false, sizeof(rect_instance_data_t), OFFSET(rect_instance_data_t, matrix[1]));
				glVertexAttribDivisor(TEXTUREDRECT_VA_MATRIX1, 1);
				glEnableVertexAttribArray(TEXTUREDRECT_VA_MATRIX1);

				glVertexAttribPointer(TEXTUREDRECT_VA_MATRIX2, 4, GL_FLOAT, false, sizeof(rect_instance_data_t), OFFSET(rect_instance_data_t, matrix[2]));
				glVertexAttribDivisor(TEXTUREDRECT_VA_MATRIX2, 1);
				glEnableVertexAttribArray(TEXTUREDRECT_VA_MATRIX2);

				glVertexAttribPointer(TEXTUREDRECT_VA_MATRIX3, 4, GL_FLOAT, false, sizeof(rect_instance_data_t), OFFSET(rect_instance_data_t, matrix[3]));
				glVertexAttribDivisor(TEXTUREDRECT_VA_MATRIX3, 1);
				glEnableVertexAttribArray(TEXTUREDRECT_VA_MATRIX3);

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_RectIndexBuffer->GetGLBufferObject());

			});

	}

	rect_instance_data_t instanceDataBuffer[1]{};

	auto worldMatrix = (float (*)[4][4])R_GetWorldMatrix();
	auto projMatrix = (float (*)[4][4])R_GetProjectionMatrix();

	Matrix4x4_Multiply(instanceDataBuffer[0].matrix, (*worldMatrix), (*projMatrix));

	size_t vertexDataSize = verticeCount * sizeof(texturedrectvertex_t);
	size_t instanceDataSize = sizeof(instanceDataBuffer);
	size_t indexDataSize = indicesCount * sizeof(uint32_t);

	CPMBRingBufferAllocation vertexAllocation;
	CPMBRingBufferAllocation instanceAllocation;
	CPMBRingBufferAllocation indexAllocation;

	if (!g_TexturedRectVertexBuffer->Allocate(vertexDataSize, vertexAllocation))
	{
		gEngfuncs.Con_DPrintf("R_DrawTexturedRect: warning - failed to allocate %d bytes for vertexData.\n", vertexDataSize);
		return;
	}

	if (!g_RectInstanceBuffer->Allocate(instanceDataSize, instanceAllocation))
	{
		gEngfuncs.Con_DPrintf("R_DrawTexturedRect: warning - failed to allocate %d bytes for instanceData.\n", instanceDataSize);
		return;
	}

	if (!g_RectIndexBuffer->Allocate(indexDataSize, indexAllocation))
	{
		gEngfuncs.Con_DPrintf("R_DrawTexturedRect: warning - failed to allocate %d bytes for indexData.\n", indexDataSize);
		return;
	}

	if (debugMetadata)
	{
		GL_BeginDebugGroupFormat("R_DrawTexturedRect - %s", debugMetadata);
	}
	else
	{
		GL_BeginDebugGroup("R_DrawTexturedRect");
	}

	if(gltexturenum > 0)
		glBindTexture(GL_TEXTURE_2D, gltexturenum);

	memcpy(vertexAllocation.ptr, verticeBuffer, vertexDataSize);
	GLuint baseVertex = (GLuint)(vertexAllocation.offset / sizeof(texturedrectvertex_t));

	memcpy(instanceAllocation.ptr, instanceDataBuffer, sizeof(instanceDataBuffer));
	GLuint baseInstance = (GLuint)(instanceAllocation.offset / sizeof(rect_instance_data_t));

	memcpy(indexAllocation.ptr, indices, indexDataSize);
	GLuint baseIndex = (GLuint)(indexAllocation.offset / sizeof(uint32_t));

	GL_BindVAO(g_DrawTexturedRectCommand.hVAO);

	if (programState & DRAW_TEXTURED_RECT_ALPHA_BLEND_ENABLED)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else if (programState & DRAW_TEXTURED_RECT_ADDITIVE_BLEND_ENABLED)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
	}
	else if (programState & DRAW_TEXTURED_RECT_ALPHA_BASED_ADDITIVE_ENABLED)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	}
	else
	{
		glDisable(GL_BLEND);
	}

	glDisable(GL_DEPTH_TEST);

	drawtexturedrect_program_t prog{};
	R_UseDrawTexturedRectProgram(programState, &prog);

	glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, BUFFER_OFFSET(baseIndex), 1, baseVertex, baseInstance);

	GL_UseProgram(0);

	glDisable(GL_BLEND);

	GL_BindVAO(0);

	if (gltexturenum > 0)
		glBindTexture(GL_TEXTURE_2D, 0);

	GL_EndDebugGroup();
}

void R_DrawTexturedRectMask(int gltexturenum, int maskTextureId, const texturedrectvertex_t* verticeBuffer, size_t verticeCount, const uint32_t* indices, size_t indicesCount, uint64_t programState, const char* debugMetadata)
{
	if (!g_DrawTexturedRectCommand.hVAO)
	{
		g_DrawTexturedRectCommand.hVAO = GL_GenVAO();

		if (!g_TexturedRectVertexBuffer)
		{
			g_TexturedRectVertexBuffer = GL_CreatePMBRingBuffer("TexturedRectVertexBuffer", 16 * 1024 * 1024, GL_ARRAY_BUFFER);
		}

		if (!g_RectInstanceBuffer)
		{
			g_RectInstanceBuffer = GL_CreatePMBRingBuffer("RectInstanceBuffer", 8 * 1024 * 1024, GL_ARRAY_BUFFER);
		}

		if (!g_RectIndexBuffer)
		{
			g_RectIndexBuffer = GL_CreatePMBRingBuffer("RectIndexBuffer", 1 * 1024 * 1024, GL_ELEMENT_ARRAY_BUFFER);
		}

		GL_BindStatesForVAO(
			g_DrawTexturedRectCommand.hVAO,
			[&]() {
				glBindBuffer(GL_ARRAY_BUFFER, g_TexturedRectVertexBuffer->GetGLBufferObject());

				glVertexAttribPointer(TEXTUREDRECT_VA_POSITION, 2, GL_FLOAT, false, sizeof(texturedrectvertex_t), OFFSET(texturedrectvertex_t, pos));
				glEnableVertexAttribArray(TEXTUREDRECT_VA_POSITION);

				glVertexAttribPointer(TEXTUREDRECT_VA_TEXCOORD, 2, GL_FLOAT, false, sizeof(texturedrectvertex_t), OFFSET(texturedrectvertex_t, texcoord));
				glEnableVertexAttribArray(TEXTUREDRECT_VA_TEXCOORD);

				glVertexAttribPointer(TEXTUREDRECT_VA_COLOR, 4, GL_FLOAT, false, sizeof(texturedrectvertex_t), OFFSET(texturedrectvertex_t, col));
				glEnableVertexAttribArray(TEXTUREDRECT_VA_COLOR);

				glBindBuffer(GL_ARRAY_BUFFER, g_RectInstanceBuffer->GetGLBufferObject());

				glVertexAttribPointer(TEXTUREDRECT_VA_MATRIX0, 4, GL_FLOAT, false, sizeof(rect_instance_data_t), OFFSET(rect_instance_data_t, matrix[0]));
				glVertexAttribDivisor(TEXTUREDRECT_VA_MATRIX0, 1);
				glEnableVertexAttribArray(TEXTUREDRECT_VA_MATRIX0);

				glVertexAttribPointer(TEXTUREDRECT_VA_MATRIX1, 4, GL_FLOAT, false, sizeof(rect_instance_data_t), OFFSET(rect_instance_data_t, matrix[1]));
				glVertexAttribDivisor(TEXTUREDRECT_VA_MATRIX1, 1);
				glEnableVertexAttribArray(TEXTUREDRECT_VA_MATRIX1);

				glVertexAttribPointer(TEXTUREDRECT_VA_MATRIX2, 4, GL_FLOAT, false, sizeof(rect_instance_data_t), OFFSET(rect_instance_data_t, matrix[2]));
				glVertexAttribDivisor(TEXTUREDRECT_VA_MATRIX2, 1);
				glEnableVertexAttribArray(TEXTUREDRECT_VA_MATRIX2);

				glVertexAttribPointer(TEXTUREDRECT_VA_MATRIX3, 4, GL_FLOAT, false, sizeof(rect_instance_data_t), OFFSET(rect_instance_data_t, matrix[3]));
				glVertexAttribDivisor(TEXTUREDRECT_VA_MATRIX3, 1);
				glEnableVertexAttribArray(TEXTUREDRECT_VA_MATRIX3);

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_RectIndexBuffer->GetGLBufferObject());

			});

	}

	rect_instance_data_t instanceDataBuffer[1]{};

	auto worldMatrix = (float (*)[4][4])R_GetWorldMatrix();
	auto projMatrix = (float (*)[4][4])R_GetProjectionMatrix();

	Matrix4x4_Multiply(instanceDataBuffer[0].matrix, (*worldMatrix), (*projMatrix));

	size_t vertexDataSize = verticeCount * sizeof(texturedrectvertex_t);
	size_t instanceDataSize = sizeof(instanceDataBuffer);
	size_t indexDataSize = indicesCount * sizeof(uint32_t);

	CPMBRingBufferAllocation vertexAllocation;
	CPMBRingBufferAllocation instanceAllocation;
	CPMBRingBufferAllocation indexAllocation;

	if (!g_TexturedRectVertexBuffer->Allocate(vertexDataSize, vertexAllocation))
	{
		gEngfuncs.Con_DPrintf("R_DrawTexturedRect: warning - failed to allocate %d bytes for vertexData.\n", vertexDataSize);
		return;
	}

	if (!g_RectInstanceBuffer->Allocate(instanceDataSize, instanceAllocation))
	{
		gEngfuncs.Con_DPrintf("R_DrawTexturedRect: warning - failed to allocate %d bytes for instanceData.\n", instanceDataSize);
		return;
	}

	if (!g_RectIndexBuffer->Allocate(indexDataSize, indexAllocation))
	{
		gEngfuncs.Con_DPrintf("R_DrawTexturedRect: warning - failed to allocate %d bytes for indexData.\n", indexDataSize);
		return;
	}

	if (debugMetadata)
	{
		GL_BeginDebugGroupFormat("R_DrawTexturedRectMask - %s", debugMetadata);
	}
	else
	{
		GL_BeginDebugGroup("R_DrawTexturedRectMask");
	}

	GL_BindTextureUnit(0, GL_TEXTURE_2D, gltexturenum);
	GL_BindTextureUnit(1, GL_TEXTURE_2D, maskTextureId);

	memcpy(vertexAllocation.ptr, verticeBuffer, vertexDataSize);
	GLuint baseVertex = (GLuint)(vertexAllocation.offset / sizeof(texturedrectvertex_t));

	memcpy(instanceAllocation.ptr, instanceDataBuffer, sizeof(instanceDataBuffer));
	GLuint baseInstance = (GLuint)(instanceAllocation.offset / sizeof(rect_instance_data_t));

	memcpy(indexAllocation.ptr, indices, indexDataSize);
	GLuint baseIndex = (GLuint)(indexAllocation.offset / sizeof(uint32_t));

	GL_BindVAO(g_DrawTexturedRectCommand.hVAO);

	if (programState & DRAW_TEXTURED_RECT_ALPHA_BLEND_ENABLED)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else if (programState & DRAW_TEXTURED_RECT_ADDITIVE_BLEND_ENABLED)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
	}
	else if (programState & DRAW_TEXTURED_RECT_ALPHA_BASED_ADDITIVE_ENABLED)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	}
	else
	{
		glDisable(GL_BLEND);
	}

	glDisable(GL_DEPTH_TEST);

	drawtexturedrect_program_t prog{};
	R_UseDrawTexturedRectProgram(programState, &prog);

	glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, BUFFER_OFFSET(baseIndex), 1, baseVertex, baseInstance);

	GL_UseProgram(0);

	glDisable(GL_BLEND);

	GL_BindVAO(0);

	GL_BindTextureUnit(1, GL_TEXTURE_2D, 0);
	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_EndDebugGroup();
}

CDrawRectCommand g_DrawFilledRectCommand;

void R_DrawFilledRect(const filledrectvertex_t* verticeBuffer, size_t verticeCount, const uint32_t* indices, size_t indicesCount, uint64_t programState, const char* debugMetadata)
{
	if (!g_DrawFilledRectCommand.hVAO)
	{
		g_DrawFilledRectCommand.hVAO = GL_GenVAO();

		if (!g_FilledRectVertexBuffer)
		{
			g_FilledRectVertexBuffer = GL_CreatePMBRingBuffer("FilledRectVertexBuffer", 16 * 1024 * 1024, GL_ARRAY_BUFFER);
		}

		if (!g_RectInstanceBuffer)
		{
			g_RectInstanceBuffer = GL_CreatePMBRingBuffer("RectInstanceBuffer", 8 * 1024 * 1024, GL_ARRAY_BUFFER);
		}

		if (!g_RectIndexBuffer)
		{
			g_RectIndexBuffer = GL_CreatePMBRingBuffer("RectIndexBuffer", 256 * 1024, GL_ELEMENT_ARRAY_BUFFER);
		}

		GL_BindStatesForVAO(
			g_DrawFilledRectCommand.hVAO,
			[&]() {
				glBindBuffer(GL_ARRAY_BUFFER, g_FilledRectVertexBuffer->GetGLBufferObject());

				glVertexAttribPointer(FILLEDRECT_VA_POSITION, 2, GL_FLOAT, false, sizeof(filledrectvertex_t), OFFSET(filledrectvertex_t, pos));
				glEnableVertexAttribArray(FILLEDRECT_VA_POSITION);

				glVertexAttribPointer(FILLEDRECT_VA_COLOR, 4, GL_FLOAT, false, sizeof(filledrectvertex_t), OFFSET(filledrectvertex_t, col));
				glEnableVertexAttribArray(FILLEDRECT_VA_COLOR);

				glBindBuffer(GL_ARRAY_BUFFER, g_RectInstanceBuffer->GetGLBufferObject());

				glVertexAttribPointer(FILLEDRECT_VA_MATRIX0, 4, GL_FLOAT, false, sizeof(rect_instance_data_t), OFFSET(rect_instance_data_t, matrix[0]));
				glVertexAttribDivisor(FILLEDRECT_VA_MATRIX0, 1);
				glEnableVertexAttribArray(FILLEDRECT_VA_MATRIX0);

				glVertexAttribPointer(FILLEDRECT_VA_MATRIX1, 4, GL_FLOAT, false, sizeof(rect_instance_data_t), OFFSET(rect_instance_data_t, matrix[1]));
				glVertexAttribDivisor(FILLEDRECT_VA_MATRIX1, 1);
				glEnableVertexAttribArray(FILLEDRECT_VA_MATRIX1);

				glVertexAttribPointer(FILLEDRECT_VA_MATRIX2, 4, GL_FLOAT, false, sizeof(rect_instance_data_t), OFFSET(rect_instance_data_t, matrix[2]));
				glVertexAttribDivisor(FILLEDRECT_VA_MATRIX2, 1);
				glEnableVertexAttribArray(FILLEDRECT_VA_MATRIX2);

				glVertexAttribPointer(FILLEDRECT_VA_MATRIX3, 4, GL_FLOAT, false, sizeof(rect_instance_data_t), OFFSET(rect_instance_data_t, matrix[3]));
				glVertexAttribDivisor(FILLEDRECT_VA_MATRIX3, 1);
				glEnableVertexAttribArray(FILLEDRECT_VA_MATRIX3);

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_RectIndexBuffer->GetGLBufferObject());
			});

	}

	rect_instance_data_t instanceDataBuffer[1]{};

	auto worldMatrix = (float (*)[4][4])R_GetWorldMatrix();
	auto projMatrix = (float (*)[4][4])R_GetProjectionMatrix();

	Matrix4x4_Multiply(instanceDataBuffer[0].matrix, (*worldMatrix), (*projMatrix));

	size_t vertexDataSize = verticeCount * sizeof(filledrectvertex_t);
	size_t instanceDataSize = sizeof(instanceDataBuffer);
	size_t indexDataSize = indicesCount * sizeof(uint32_t);

	CPMBRingBufferAllocation vertexAllocation;
	CPMBRingBufferAllocation instanceAllocation;
	CPMBRingBufferAllocation indexAllocation;

	if (!g_FilledRectVertexBuffer->Allocate(vertexDataSize, vertexAllocation))
	{
		return;
	}

	if (!g_RectInstanceBuffer->Allocate(instanceDataSize, instanceAllocation))
	{
		return;
	}

	if (!g_RectIndexBuffer->Allocate(indexDataSize, indexAllocation))
	{
		return;
	}

	if (debugMetadata)
	{
		GL_BeginDebugGroupFormat("R_DrawFilledRect - %s", debugMetadata);
	}
	else
	{
		GL_BeginDebugGroup("R_DrawFilledRect");
	}

	glDisable(GL_DEPTH_TEST);

	memcpy(vertexAllocation.ptr, verticeBuffer, vertexDataSize);
	GLuint baseVertex = (GLuint)(vertexAllocation.offset / sizeof(filledrectvertex_t));

	memcpy(instanceAllocation.ptr, instanceDataBuffer, sizeof(instanceDataBuffer));
	GLuint baseInstance = (GLuint)(instanceAllocation.offset / sizeof(rect_instance_data_t));

	memcpy(indexAllocation.ptr, indices, indexDataSize);
	GLuint baseIndex = (GLuint)(indexAllocation.offset / sizeof(uint32_t));

	GL_BindVAO(g_DrawFilledRectCommand.hVAO);

	if (programState & DRAW_FILLED_RECT_ALPHA_BLEND_ENABLED)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else if (programState & DRAW_FILLED_RECT_ADDITIVE_BLEND_ENABLED)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
	}
	else if (programState & DRAW_FILLED_RECT_ALPHA_BASED_ADDITIVE_ENABLED)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	}
	else if (programState & DRAW_FILLED_RECT_ZERO_SRC_ALPHA_BLEND_ENABLED)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_ZERO, GL_SRC_ALPHA);
	}
	else
	{
		glDisable(GL_BLEND);
	}

	drawfilledrect_program_t prog{};
	R_UseDrawFilledRectProgram(programState, &prog);

	if (programState & DRAW_FILLED_RECT_LINE_ENABLED)
	{
		glDrawElementsInstancedBaseVertexBaseInstance(GL_LINES, indicesCount, GL_UNSIGNED_INT, BUFFER_OFFSET(baseIndex), 1, baseVertex, baseInstance);
	}
	else
	{
		glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, BUFFER_OFFSET(baseIndex), 1, baseVertex, baseInstance);
	}

	GL_UseProgram(0);

	glDisable(GL_BLEND);

	GL_BindVAO(0);

	GL_EndDebugGroup();
}

void R_DrawTexturedQuad(int gltexturenum, int x0, int y0, int x1, int y1, const float* color4v, uint64_t programState, const char* debugMetadata)
{
	texturedrectvertex_t vertices[4];

	vertices[0].col[0] = color4v[0];
	vertices[0].col[1] = color4v[1];
	vertices[0].col[2] = color4v[2];
	vertices[0].col[3] = color4v[3];
	vertices[0].texcoord[0] = 0;
	vertices[0].texcoord[1] = 1;
	vertices[0].pos[0] = x0;
	vertices[0].pos[1] = y1;

	vertices[1].col[0] = color4v[0];
	vertices[1].col[1] = color4v[1];
	vertices[1].col[2] = color4v[2];
	vertices[1].col[3] = color4v[3];
	vertices[1].texcoord[0] = 1;
	vertices[1].texcoord[1] = 1;
	vertices[1].pos[0] = x1;
	vertices[1].pos[1] = y1;

	vertices[2].col[0] = color4v[0];
	vertices[2].col[1] = color4v[1];
	vertices[2].col[2] = color4v[2];
	vertices[2].col[3] = color4v[3];
	vertices[2].texcoord[0] = 1;
	vertices[2].texcoord[1] = 0;
	vertices[2].pos[0] = x1;
	vertices[2].pos[1] = y0;

	vertices[3].col[0] = color4v[0];
	vertices[3].col[1] = color4v[1];
	vertices[3].col[2] = color4v[2];
	vertices[3].col[3] = color4v[3];
	vertices[3].texcoord[0] = 0;
	vertices[3].texcoord[1] = 0;
	vertices[3].pos[0] = x0;
	vertices[3].pos[1] = y0;

	const uint32_t indices[] = { 0,1,2,2,3,0 };

	R_DrawTexturedRect(gltexturenum, vertices, _countof(vertices), indices, _countof(indices), programState, debugMetadata);
}

void R_DrawTexturedQuadMask(int gltexturenum, int maskTextureId, int x0, int y0, int x1, int y1, const float* color4v, uint64_t programState, const char* debugMetadata)
{
	texturedrectvertex_t vertices[4];

	vertices[0].col[0] = color4v[0];
	vertices[0].col[1] = color4v[1];
	vertices[0].col[2] = color4v[2];
	vertices[0].col[3] = color4v[3];
	vertices[0].texcoord[0] = 0;
	vertices[0].texcoord[1] = 1;
	vertices[0].pos[0] = x0;
	vertices[0].pos[1] = y1;

	vertices[1].col[0] = color4v[0];
	vertices[1].col[1] = color4v[1];
	vertices[1].col[2] = color4v[2];
	vertices[1].col[3] = color4v[3];
	vertices[1].texcoord[0] = 1;
	vertices[1].texcoord[1] = 1;
	vertices[1].pos[0] = x1;
	vertices[1].pos[1] = y1;

	vertices[2].col[0] = color4v[0];
	vertices[2].col[1] = color4v[1];
	vertices[2].col[2] = color4v[2];
	vertices[2].col[3] = color4v[3];
	vertices[2].texcoord[0] = 1;
	vertices[2].texcoord[1] = 0;
	vertices[2].pos[0] = x1;
	vertices[2].pos[1] = y0;

	vertices[3].col[0] = color4v[0];
	vertices[3].col[1] = color4v[1];
	vertices[3].col[2] = color4v[2];
	vertices[3].col[3] = color4v[3];
	vertices[3].texcoord[0] = 0;
	vertices[3].texcoord[1] = 0;
	vertices[3].pos[0] = x0;
	vertices[3].pos[1] = y0;

	const uint32_t indices[] = { 0,1,2,2,3,0 };

	R_DrawTexturedRectMask(gltexturenum, maskTextureId, vertices, _countof(vertices), indices, _countof(indices), programState, debugMetadata);
}

void R_DrawFilledQuad(int x0, int y0, int x1, int y1, const float* color4v, uint64_t programState, const char* debugMetadata)
{
	filledrectvertex_t vertices[4];

	vertices[0].col[0] = color4v[0];
	vertices[0].col[1] = color4v[1];
	vertices[0].col[2] = color4v[2];
	vertices[0].col[3] = color4v[3];
	vertices[0].pos[0] = x0;
	vertices[0].pos[1] = y1;

	vertices[1].col[0] = color4v[0];
	vertices[1].col[1] = color4v[1];
	vertices[1].col[2] = color4v[2];
	vertices[1].col[3] = color4v[3];
	vertices[1].pos[0] = x1;
	vertices[1].pos[1] = y1;

	vertices[2].col[0] = color4v[0];
	vertices[2].col[1] = color4v[1];
	vertices[2].col[2] = color4v[2];
	vertices[2].col[3] = color4v[3];
	vertices[2].pos[0] = x1;
	vertices[2].pos[1] = y0;

	vertices[3].col[0] = color4v[0];
	vertices[3].col[1] = color4v[1];
	vertices[3].col[2] = color4v[2];
	vertices[3].col[3] = color4v[3];
	vertices[3].pos[0] = x0;
	vertices[3].pos[1] = y0;

	const uint32_t indices[] = { 0,1,2,2,3,0 };

	R_DrawFilledRect(vertices, _countof(vertices), indices, _countof(indices), programState, debugMetadata);
}
/*
	Purpose: Blit src FBO to screen, the current rendering FBO will be undefined, you must bind correct FBO again after this
*/

void GL_BlitFrameFufferToScreen(FBO_Container_t *src)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->s_hBackBufferFBO);

	glBlitFramebuffer(0, 0, src->iWidth, src->iHeight, 0, 0, R_GetSwapChainWidth(), R_GetSwapChainHeight(), GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

/*
	Purpose: Blit color of src FBO to dst, the current rendering FBO will be undefined, you must bind correct FBO again after this
*/
void GL_BlitFrameBufferToFrameBufferColorOnly(FBO_Container_t *src, FBO_Container_t *dst)
{
	if (src == dst)
		return;

	GL_BeginDebugGroupFormat("GL_BlitFrameBufferToFrameBufferColorOnly - %s to %s", GL_GetFrameBufferName(src), GL_GetFrameBufferName(dst));

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->s_hBackBufferFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->s_hBackBufferFBO);

	glBlitFramebuffer(0, 0, src->iWidth, src->iHeight, 0, 0, dst->iWidth, dst->iHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	GL_EndDebugGroup();
}

/*
	Purpose: Blit color and depth of src FBO to dst, the current rendering FBO will be undefined, you must bind correct FBO again after this
*/
void GL_BlitFrameBufferToFrameBufferColorDepth(FBO_Container_t *src, FBO_Container_t *dst)
{
	if (src == dst)
		return;

	GL_BeginDebugGroupFormat("GL_BlitFrameBufferToFrameBufferColorDepth - %s to %s", GL_GetFrameBufferName(src), GL_GetFrameBufferName(dst));

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->s_hBackBufferFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->s_hBackBufferFBO);

	glBlitFramebuffer(0, 0, src->iWidth, src->iHeight, 0, 0, dst->iWidth, dst->iHeight, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	GL_EndDebugGroup();
}

/*
	Purpose: Blit color, depth and stencil of src FBO to dst, the current rendering FBO will be undefined, you must bind correct FBO again after this
*/
void GL_BlitFrameBufferToFrameBufferColorDepthStencil(FBO_Container_t* src, FBO_Container_t* dst)
{
	if (src == dst)
		return;

	GL_BeginDebugGroupFormat("GL_BlitFrameBufferToFrameBufferColorDepthStencil - %s to %s", GL_GetFrameBufferName(src), GL_GetFrameBufferName(dst));

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->s_hBackBufferFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->s_hBackBufferFBO);

	glBlitFramebuffer(0, 0, src->iWidth, src->iHeight, 0, 0, dst->iWidth, dst->iHeight, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

	GL_EndDebugGroup();
}

/*
	Purpose: Blit depth of src FBO to dst, the current rendering FBO will be undefined, you must bind correct FBO again after this
*/
void GL_BlitFrameBufferToFrameBufferDepthOnly(FBO_Container_t* src, FBO_Container_t* dst)
{
	GL_BeginDebugGroupFormat("GL_BlitFrameBufferToFrameBufferDepthOnly - %s to %s", GL_GetFrameBufferName(src), GL_GetFrameBufferName(dst));

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->s_hBackBufferFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->s_hBackBufferFBO);

	glBlitFramebuffer(0, 0, src->iWidth, src->iHeight, 0, 0, dst->iWidth, dst->iHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	GL_EndDebugGroup();
}

/*
	Purpose: Blit stencil of src FBO to dst, the current rendering FBO will be undefined, you must bind correct FBO again after this
*/
void GL_BlitFrameBufferToFrameBufferStencilOnly(FBO_Container_t* src, FBO_Container_t* dst)
{
	GL_BeginDebugGroupFormat("GL_BlitFrameBufferToFrameBufferStencilOnly - %s to %s", GL_GetFrameBufferName(src), GL_GetFrameBufferName(dst));

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->s_hBackBufferFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->s_hBackBufferFBO);

	glBlitFramebuffer(0, 0, src->iWidth, src->iHeight, 0, 0, dst->iWidth, dst->iHeight, GL_STENCIL_BUFFER_BIT, GL_NEAREST);

	GL_EndDebugGroup();
}

/*
	Purpose: Blit depth and stencil of src FBO to dst, the current rendering FBO will be undefined, you must bind correct FBO again after this
*/
void GL_BlitFrameBufferToFrameBufferDepthStencil(FBO_Container_t* src, FBO_Container_t* dst)
{
	GL_BeginDebugGroupFormat("GL_BlitFrameBufferToFrameBufferDepthStencil - %s to %s", GL_GetFrameBufferName(src), GL_GetFrameBufferName(dst));

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->s_hBackBufferFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->s_hBackBufferFBO);

	glBlitFramebuffer(0, 0, src->iWidth, src->iHeight, 0, 0, dst->iWidth, dst->iHeight, GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

	GL_EndDebugGroup();
}

/*
	Purpose: Down sample from src_color to dst, the current rendering FBO is set to dst
*/
void R_DownSample(FBO_Container_t *src_color, FBO_Container_t* src_stencil, FBO_Container_t *dst, bool bUseFilter2x2, bool bUseStencilFilter)
{
	GL_BeginDebugGroupFormat("R_DownSample - %s to %s", GL_GetFrameBufferName(src_color), GL_GetFrameBufferName(dst));

	GL_BindFrameBuffer(dst);

	GL_Set2DEx(0, 0, dst->iWidth, dst->iHeight);

	vec4_t vecClearColor = { 0, 0, 0, 0 };
	GL_ClearColor(vecClearColor);
	GL_ClearStencil(0xFF);

	if (bUseFilter2x2)
	{
		GL_UseProgram(pp_downsample2x2.program);
		glUniform2f(0, 2.0f / src_color->iWidth, 2.0f / src_color->iHeight);
	}
	else
	{
		GL_UseProgram(pp_downsample.program);
	}

	GL_BindVAO(r_empty_vao);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, src_color->s_hBackBufferTex);

	if (bUseStencilFilter && src_stencil)
	{
		GL_BlitFrameBufferToFrameBufferStencilOnly(src_stencil, dst);

		GL_BeginStencilCompareNotEqual(STENCIL_MASK_NO_BLOOM, STENCIL_MASK_NO_BLOOM);

		glDrawArrays(GL_TRIANGLES, 0, 3);

		GL_EndStencil();
	}
	else
	{
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_BindVAO(0);

	GL_UseProgram(0);

	GL_Finish2D();

	GL_EndDebugGroup();
}

void R_LuminPass(FBO_Container_t *src, FBO_Container_t *dst, int type)
{
	GL_BeginDebugGroupFormat("R_LuminPass - write to %s, type = %d", GL_GetFrameBufferName(dst), type);

	GL_BindFrameBuffer(dst);

	GL_Set2DEx(0, 0, dst->iWidth, dst->iHeight);

	vec4_t vecClearColor = { 0, 0, 0, 0 };
	GL_ClearColor(vecClearColor);

	if(type == LUMPASS_LOG)
	{
		GL_UseProgram(pp_luminlog.program);
		glUniform2f(0, 2.0f / src->iWidth, 2.0f / src->iHeight);
	}
	else if(type == LUMPASS_EXP)
	{
		GL_UseProgram(pp_luminexp.program);
		glUniform2f(0, 2.0f / src->iWidth, 2.0f / src->iHeight);
	}
	else
	{
		GL_UseProgram(pp_lumindown.program);
		glUniform2f(0, 2.0f / src->iWidth, 2.0f / src->iHeight);
	}

	GL_BindVAO(r_empty_vao);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, src->s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_BindVAO(0);

	GL_UseProgram(0);

	GL_Finish2D();

	GL_EndDebugGroup();
}

void R_LuminAdaptation(FBO_Container_t *src, FBO_Container_t *dst, FBO_Container_t *ada, double frametime)
{
	GL_BeginDebugGroupFormat("R_LuminAdaptation - write to %s", GL_GetFrameBufferName(dst));

	GL_BindFrameBuffer(dst);

	GL_Set2DEx(0, 0, dst->iWidth, dst->iHeight);

	vec4_t vecClearColor = { 0, 0, 0, 0 };
	GL_ClearColor(vecClearColor);

	GL_UseProgram(pp_luminadapt.program);

	glUniform1f(0, frametime * math_clamp(r_hdr_adaptation->GetValue(), 0.1, 100));

	GL_BindVAO(r_empty_vao);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, src->s_hBackBufferTex);

	GL_BindTextureUnit(1, GL_TEXTURE_2D, ada->s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_BindTextureUnit(1, GL_TEXTURE_2D, 0);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_BindVAO(0);

	GL_UseProgram(0);

	GL_Finish2D();

	GL_EndDebugGroup();
}

void R_BrightPass(FBO_Container_t *src, FBO_Container_t *dst, FBO_Container_t *lum)
{
	GL_BeginDebugGroupFormat("R_BrightPass - %s to %s", GL_GetFrameBufferName(src), GL_GetFrameBufferName(dst));

	GL_BindFrameBuffer(dst);

	GL_Set2DEx(0, 0, dst->iWidth, dst->iHeight);

	vec4_t vecClearColor = { 0, 0, 0, 0 };
	GL_ClearColor(vecClearColor);

	GL_UseProgram(pp_brightpass.program);

	GL_BindVAO(r_empty_vao);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, src->s_hBackBufferTex);

	GL_BindTextureUnit(1, GL_TEXTURE_2D, lum->s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_BindTextureUnit(1, GL_TEXTURE_2D, 0);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_BindVAO(0);

	GL_UseProgram(0);

	GL_Finish2D();

	GL_EndDebugGroup();
}

void R_BlurPass(FBO_Container_t *src, FBO_Container_t *dst, float scale, bool vertical)
{
	GL_BeginDebugGroupFormat("R_BlurPass - %s to %s", GL_GetFrameBufferName(src), GL_GetFrameBufferName(dst));

	GL_BindFrameBuffer(dst);

	GL_Set2DEx(0, 0, dst->iWidth, dst->iHeight);

	vec4_t vecClearColor = { 0, 0, 0, 0 };
	GL_ClearColor(vecClearColor);

	if (vertical)
	{
		GL_UseProgram(pp_gaussianblurv.program);
		glUniform1f(0, scale / src->iHeight);
	}
	else
	{
		GL_UseProgram(pp_gaussianblurh.program);
		glUniform1f(0, scale / src->iWidth);
	}

	GL_BindVAO(r_empty_vao);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, src->s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_BindVAO(0);

	GL_UseProgram(0);

	GL_Finish2D();

	GL_EndDebugGroup();
}

void R_BrightAccum(FBO_Container_t *blur1, FBO_Container_t *blur2, FBO_Container_t *blur3, FBO_Container_t *dst)
{
	GL_BeginDebugGroupFormat("R_BrightAccum - write to %s", GL_GetFrameBufferName(dst));

	GL_BindFrameBuffer(dst);

	GL_Set2DEx(0, 0, dst->iWidth, dst->iHeight);

	vec4_t vecClearColor = { 0, 0, 0, 0 };
	GL_ClearColor(vecClearColor);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	GL_UseProgram(pp_downsample.program);
	
	GL_BindVAO(r_empty_vao);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, blur1->s_hBackBufferTex);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, blur2->s_hBackBufferTex);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, blur3->s_hBackBufferTex);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_BindVAO(0);

	glDisable(GL_BLEND);

	GL_UseProgram(0);

	GL_Finish2D();

	GL_EndDebugGroup();
}

void R_ToneMapping(FBO_Container_t *src, FBO_Container_t *dst, FBO_Container_t *blur, FBO_Container_t *lum)
{
	GL_BeginDebugGroupFormat("R_ToneMapping - write to %s", GL_GetFrameBufferName(dst));

	GL_BindFrameBuffer(dst);

	GL_Set2DEx(0, 0, dst->iWidth, dst->iHeight);

	vec4_t vecClearColor = { 0, 0, 0, 0 };
	GL_ClearColor(vecClearColor);

	GL_UseProgram(pp_tonemap.program);
	glUniform1f(0, math_clamp(r_hdr_blurwidth->GetValue(), 0, 1));
	glUniform1f(1, math_clamp(r_hdr_darkness->GetValue(), 0.001, 10));
	glUniform1f(2, math_clamp(r_hdr_exposure->GetValue(), 0.001, 10));

	GL_BindVAO(r_empty_vao);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, src->s_hBackBufferTex);

	GL_BindTextureUnit(1, GL_TEXTURE_2D, blur->s_hBackBufferTex);

	GL_BindTextureUnit(2, GL_TEXTURE_2D, lum->s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_BindTextureUnit(2, GL_TEXTURE_2D, 0);

	GL_BindTextureUnit(1, GL_TEXTURE_2D, 0);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_BindVAO(0);

	GL_UseProgram(0);

	GL_Finish2D();

	GL_EndDebugGroup();
}

bool R_IsHDREnabled(void)
{
	if (!r_hdr->value)
		return false;

	if ((*r_refdef.onlyClientDraws))
		return false;

	if (R_IsRenderingPortal())
		return false;

	if (CL_IsDevOverviewMode())
		return false;

	return true;
}

void R_HDR(FBO_Container_t* src_color, FBO_Container_t* src_stencil, FBO_Container_t* dst)
{
	GL_BeginDebugGroupFormat("R_HDR - color=%s, stencil=%s, dst=%s", GL_GetFrameBufferName(src_color), GL_GetFrameBufferName(src_stencil), GL_GetFrameBufferName(dst));

	R_DownSample(src_color, src_stencil, &s_DownSampleFBO[0], true, true);//(1->1/4)
	R_DownSample(&s_DownSampleFBO[0], nullptr, &s_DownSampleFBO[1], true, false);//(1/4)->(1/16)

	//Log Luminance DownSample from .. (RGB16F to R32F)
	R_LuminPass(&s_DownSampleFBO[1], &s_LuminFBO[0], LUMPASS_LOG);//(1/16)->64x64

	//Luminance DownSample from..
	R_LuminPass(&s_LuminFBO[0], &s_LuminFBO[1], LUMPASS_DOWN);//64x64->16x16
	R_LuminPass(&s_LuminFBO[1], &s_LuminFBO[2], LUMPASS_DOWN);//16x16->4x4

	//exp Luminance DownSample from..
	R_LuminPass(&s_LuminFBO[2], &s_Lumin1x1FBO[2], LUMPASS_EXP);//4x4->1x1

	//Luminance Adaptation
	R_LuminAdaptation(&s_Lumin1x1FBO[2], &s_Lumin1x1FBO[!last_luminance], &s_Lumin1x1FBO[last_luminance], (*cl_time) - (*cl_oldtime));
	last_luminance = !last_luminance;

	//Bright Pass (with 1/16)
	R_BrightPass(&s_DownSampleFBO[1], &s_BrightPassFBO, &s_Lumin1x1FBO[last_luminance]);

	//Gaussian Blur Pass (with bright pass)
	R_BlurPass(&s_BrightPassFBO, &s_BlurPassFBO[0][0], 1, false);
	R_BlurPass(&s_BlurPassFBO[0][0], &s_BlurPassFBO[0][1], 1, true);
	//Blur again and downsample from 1/16 to 1/32
	R_BlurPass(&s_BlurPassFBO[0][1], &s_BlurPassFBO[1][0], 1, false);
	R_BlurPass(&s_BlurPassFBO[1][0], &s_BlurPassFBO[1][1], 1, true);
	//Blur again and downsample from 1/32 to 1/64
	R_BlurPass(&s_BlurPassFBO[1][1], &s_BlurPassFBO[2][0], 1, false);
	R_BlurPass(&s_BlurPassFBO[2][0], &s_BlurPassFBO[2][1], 1, true);

	//Accumulate all blurred textures
	R_BrightAccum(&s_BlurPassFBO[0][1], &s_BlurPassFBO[1][1], &s_BlurPassFBO[2][1], &s_BrightAccumFBO);

	//Tone mapping
	R_ToneMapping(src_color, &s_ToneMapFBO, &s_BrightAccumFBO, &s_Lumin1x1FBO[last_luminance]);

	GL_BlitFrameBufferToFrameBufferColorOnly(&s_ToneMapFBO, dst);

	GL_EndDebugGroup();
}

bool R_IsFXAAEnabled(void)
{
	if (!r_fxaa->value)
		return false;

	if ((*r_refdef.onlyClientDraws))
		return false;

	if (R_IsRenderingPortal())
		return false;

	if (CL_IsDevOverviewMode())
		return false;

	return true;
}

void R_FXAA(FBO_Container_t* src, FBO_Container_t* dst)
{
	GL_BeginDebugGroupFormat("R_FXAA - %s to %s", GL_GetFrameBufferName(src), GL_GetFrameBufferName(dst));

	GL_BindFrameBuffer(dst);

	GL_Set2DEx(0, 0, dst->iWidth, dst->iHeight);

	GL_BindVAO(r_empty_vao);

	GL_UseProgram(pp_fxaa.program);
	glUniform1f(0, dst->iWidth);
	glUniform1f(1, dst->iHeight);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, src->s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_UseProgram(0);

	GL_BindVAO(0);

	GL_Finish2D();

	GL_EndDebugGroup();
}

bool R_IsUnderWaterEffectEnabled(void)
{
	if (!r_under_water_effect->value)
		return false;

	if ((*r_refdef.onlyClientDraws))
		return false;

	if (R_IsRenderingPortal())
		return false;

	if (CL_IsDevOverviewMode())
		return false;

	if ((*cl_waterlevel) > 2 && !(*r_refdef.onlyClientDraws))
	{

	}
	else
	{
		return false;
	}

	return true;
}

void R_UnderWaterEffect(FBO_Container_t* src, FBO_Container_t* dst)
{
	GL_BeginDebugGroupFormat("R_UnderWaterEffect - %s to %s", GL_GetFrameBufferName(src), GL_GetFrameBufferName(dst));

	GL_BindFrameBuffer(dst);

	GL_Set2DEx(0, 0, dst->iWidth, dst->iHeight);

	GL_UseProgram(under_water_effect.program);
	glUniform1f(0, r_under_water_effect_wave_amount->value);
	glUniform1f(1, r_under_water_effect_wave_speed->value);
	glUniform1f(2, r_under_water_effect_wave_size->value);

	GL_BindVAO(r_empty_vao);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, src->s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_BindVAO(0);

	GL_UseProgram(0); 

	GL_Finish2D();
	
	GL_EndDebugGroup();
}

bool R_IsGammaBlendEnabled()
{
	if (r_gamma_blend->value > 0)
	{
		if ((*r_refdef.onlyClientDraws))
			return false;

		if (R_IsRenderingShadowView())
			return false;

		if (R_IsRenderingWaterView())
			return false;

		if (R_IsRenderingPortal())
			return false;

		if (CL_IsDevOverviewMode())
			return false;

		return true;
	}
	return false;
}

void R_GammaCorrection(FBO_Container_t* src, FBO_Container_t* dst)
{
	GL_BeginDebugGroupFormat("R_GammaCorrection - %s to %s", GL_GetFrameBufferName(src), GL_GetFrameBufferName(dst));

	GL_BindFrameBuffer(dst);

	GL_Set2DEx(0, 0, dst->iWidth, dst->iHeight);

	GL_UseProgram(gamma_correction.program);

	GL_BindVAO(r_empty_vao);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, src->s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_BindVAO(0);

	GL_UseProgram(0);

	GL_Finish2D();

	GL_EndDebugGroup();
}

void R_GammaUncorrection(FBO_Container_t *src, FBO_Container_t* dst)
{
	GL_BeginDebugGroupFormat("R_GammaUncorrection - %s to %s", GL_GetFrameBufferName(src), GL_GetFrameBufferName(dst));

	GL_BindFrameBuffer(dst);

	GL_Set2DEx(0, 0, dst->iWidth, dst->iHeight);

	GL_UseProgram(gamma_uncorrection.program);

	GL_BindVAO(r_empty_vao);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, src->s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_BindVAO(0);

	GL_UseProgram(0);

	GL_Finish2D();

	GL_EndDebugGroup();
}

void R_ClearOITBuffer(void)
{
	GL_BeginDebugGroup("R_ClearOITBuffer");

	GL_UseProgram(oitbuffer_clear.program);

	GL_BindVAO(r_empty_vao);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_BindVAO(0);

	GL_UseProgram(0);

	GLuint val = 0;
	glClearNamedBufferData(g_WorldSurfaceRenderer.hOITAtomicSSBO, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, (const void*)&val);

	GL_EndDebugGroup();
}

void R_BlendOITBuffer(FBO_Container_t* src, FBO_Container_t* dst)
{
	GL_BeginDebugGroupFormat("R_BlendOITBuffer - %s to %s", GL_GetFrameBufferName(src), GL_GetFrameBufferName(dst));

	GL_BindFrameBuffer(dst);
	
	GL_UseProgram(blit_oitblend.program);	

	GL_BindVAO(r_empty_vao);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, src->s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_BindVAO(0);

	GL_UseProgram(0);

	GL_EndDebugGroup();
}

void R_LinearizeDepth(FBO_Container_t *src, FBO_Container_t* dst)
{
	GL_BeginDebugGroupFormat("R_LinearizeDepth - %s to %s", GL_GetFrameBufferName(src), GL_GetFrameBufferName(dst));

	GL_BindFrameBuffer(dst);

	GL_Set2DEx(0, 0, dst->iWidth, dst->iHeight);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	GL_UseProgram(depth_linearize.program);

	glUniform4f(0, r_znear * r_zfar, r_znear - r_zfar, r_zfar, r_ortho ? 0 : 1);
	
	GL_BindVAO(r_empty_vao);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, src->s_hBackBufferDepthTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_BindVAO(0);

	GL_UseProgram(0);

	GL_Finish2D();

	GL_EndDebugGroup();
}

bool R_IsAmbientOcclusionEnabled(void)
{
	if (!r_ssao->value)
		return false;

	if ((*r_refdef.onlyClientDraws))
		return false;

	if (R_IsRenderingShadowView())
		return false;

	if (R_IsRenderingWaterView())
		return false;

	if (R_IsRenderingPortal())
		return false;

	if (CL_IsDevOverviewMode())
		return false;

	return true;
}

void R_AmbientOcclusion_CalcPass(FBO_Container_t* src, FBO_Container_t* dst, float meters2viewspace)
{
	GL_BeginDebugGroup("R_AmbientOcclusion_CalcPass");

	//Prepare parameters
	const float* ProjMatrix = r_projection_matrix;

	float projInfoPerspective[] = {
		2.0f / (ProjMatrix[4 * 0 + 0]),       // (x) * (R - L)/N
		2.0f / (ProjMatrix[4 * 1 + 1]),       // (y) * (T - B)/N
		-(1.0f - ProjMatrix[4 * 2 + 0]) / ProjMatrix[4 * 0 + 0], // L/N
		-(1.0f + ProjMatrix[4 * 2 + 1]) / ProjMatrix[4 * 1 + 1], // B/N
	};

	float projScale = float(src->iHeight) / (tanf(r_yfov * M_PI * 0.5f / 180.f) * 2.0f);

	// radius
	float R = r_ssao_radius->GetValue() * meters2viewspace;
	auto R2 = R * R;
	auto NegInvR2 = -1.0f / R2;
	auto RadiusToScreen = R * 0.5f * projScale;

	// ao
	auto PowExponent = max(r_ssao_intensity->GetValue(), 0.0f);
	auto NDotVBias = min(max(0.0f, r_ssao_bias->GetValue()), 1.0f);
	auto AOMultiplier = 1.0f / (1.0f - NDotVBias);

	// resolution
	int quarterWidth = ((src->iWidth + 3) / 4);
	int quarterHeight = ((src->iHeight + 3) / 4);

	vec2_t InvQuarterResolution;
	InvQuarterResolution[0] = 1.0f / float(quarterWidth);
	InvQuarterResolution[1] = 1.0f / float(quarterHeight);

	vec2_t InvFullResolution;
	InvFullResolution[0] = 1.0f / float(src->iWidth);
	InvFullResolution[1] = 1.0f / float(src->iHeight);

	GL_BindFrameBuffer(&s_HBAOCalcFBO);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	GL_Set2DEx(0, 0, dst->iWidth, dst->iHeight);

	//Setup for hbao_calc
	if (r_fog_mode == GL_LINEAR && R_IsRenderingFog())
	{
		GL_UseProgram(hbao_calc_blur_fog.program);
		glUniform4fv(hbao_calc_blur_fog.control_projInfo, 1, projInfoPerspective);
		glUniform2fv(hbao_calc_blur_fog.control_InvFullResolution, 1, InvFullResolution);
		glUniform2fv(hbao_calc_blur_fog.control_InvQuarterResolution, 1, InvQuarterResolution);
		glUniform1i(hbao_calc_blur_fog.control_projOrtho, 0);
		glUniform1f(hbao_calc_blur_fog.control_RadiusToScreen, RadiusToScreen);
		glUniform1f(hbao_calc_blur_fog.control_AOMultiplier, AOMultiplier);
		glUniform1f(hbao_calc_blur_fog.control_NDotVBias, NDotVBias);
		glUniform1f(hbao_calc_blur_fog.control_NegInvR2, NegInvR2);
		glUniform1f(hbao_calc_blur_fog.control_PowExponent, PowExponent);
		glUniform2f(hbao_calc_blur_fog.control_Fog, r_fog_control[0], r_fog_control[1]);
	}
	else
	{
		GL_UseProgram(hbao_calc_blur.program);
		glUniform4fv(hbao_calc_blur.control_projInfo, 1, projInfoPerspective);
		glUniform2fv(hbao_calc_blur.control_InvFullResolution, 1, InvFullResolution);
		glUniform2fv(hbao_calc_blur.control_InvQuarterResolution, 1, InvQuarterResolution);
		glUniform1i(hbao_calc_blur.control_projOrtho, 0);
		glUniform1f(hbao_calc_blur.control_RadiusToScreen, RadiusToScreen);
		glUniform1f(hbao_calc_blur.control_AOMultiplier, AOMultiplier);
		glUniform1f(hbao_calc_blur.control_NDotVBias, NDotVBias);
		glUniform1f(hbao_calc_blur.control_NegInvR2, NegInvR2);
		glUniform1f(hbao_calc_blur.control_PowExponent, PowExponent);
	}

	GL_BindVAO(r_empty_vao);

	//Texture unit 0 = linearized depth
	GL_BindTextureUnit(0, GL_TEXTURE_2D, src->s_hBackBufferTex);

	//Texture unit 1 = random texture
	GL_BindTextureUnit(1, GL_TEXTURE_2D, hbao_randomview[0]);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	//Texture unit 1 = random texture
	GL_BindTextureUnit(1, GL_TEXTURE_2D, 0);

	//Texture unit 0 = linearized depth
	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_BindVAO(0);

	GL_UseProgram(0);

	GL_Finish2D();

	GL_EndDebugGroup();
}

void R_AmbientOcclusion_BlurPass(FBO_Container_t* src, FBO_Container_t* dst, float meters2viewspace)
{
	GL_BeginDebugGroup("R_AmbientOcclusion_BlurPass");

	//SSAO blur stage
	GL_BindFrameBuffer(&s_HBAOCalcFBO);

	//Write to HBAO texture2
	glDrawBuffer(GL_COLOR_ATTACHMENT1);

	GL_Set2DEx(0, 0, dst->iWidth, dst->iHeight);

	GL_UseProgram(hbao_blur.program);
	glUniform1f(0, r_ssao_blur_sharpness->GetValue() / meters2viewspace);
	glUniform2f(1, 1.0f / (float)(src->iWidth), 0);

	GL_BindVAO(r_empty_vao);

	//Texture unit 0 = s_HBAOCalcFBO.s_hBackBufferTex
	GL_BindTextureUnit(0, GL_TEXTURE_2D, s_HBAOCalcFBO.s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_BindVAO(0);

	GL_UseProgram(0);

	GL_Finish2D();

	GL_EndDebugGroup();
}

void R_AmbientOcclusion_WritePass(FBO_Container_t* src, FBO_Container_t* dst, float meters2viewspace)
{
	GL_BeginDebugGroup("R_AmbientOcclusion_WritePass");

	//Write to main framebuffer or GBuffer lightmap channel
	GL_BindFrameBuffer(dst);

	//write to GBuffer lightmap channel
	if (dst == &s_GBufferFBO)
	{
		glDrawBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_INDEX_LIGHTMAP);
	}
	else
	{
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
	}

	GL_Set2DEx(0, 0, dst->iWidth, dst->iHeight);

	//0 for occluded color, 1 for non-occluded color
	glEnable(GL_BLEND);
	glBlendFunc(GL_ZERO, GL_SRC_COLOR);

	//Only draw on non-flatshade surfaces
	GL_BeginStencilCompareNotEqual(STENCIL_MASK_HAS_FLATSHADE, STENCIL_MASK_HAS_FLATSHADE);

	GL_UseProgram(hbao_blur2.program);

	glUniform1f(0, r_ssao_blur_sharpness->GetValue() / meters2viewspace);
	glUniform2f(1, 0, 1.0f / float(glheight));

	GL_BindVAO(r_empty_vao);

	//Texture unit 0 = s_HBAOCalcFBO.s_hBackBufferTex2
	GL_BindTextureUnit(0, GL_TEXTURE_2D, s_HBAOCalcFBO.s_hBackBufferTex2);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_BindVAO(0);

	GL_UseProgram(0);

	GL_EndStencil();

	glDisable(GL_BLEND);

	GL_Finish2D();

	GL_EndDebugGroup();
}

void R_AmbientOcclusion(FBO_Container_t* src, FBO_Container_t* dst)
{
	GL_BeginDebugGroupFormat("R_AmbientOcclusion - %s to %s", GL_GetFrameBufferName(src), GL_GetFrameBufferName(dst));

	float meters2viewspace = 1.0f;

	R_AmbientOcclusion_CalcPass(src, dst, meters2viewspace);

	R_AmbientOcclusion_BlurPass(src, dst, meters2viewspace);

	R_AmbientOcclusion_WritePass(src, dst, meters2viewspace);

	GL_EndDebugGroup();
}

void R_CopyColor(FBO_Container_t* src, FBO_Container_t* dst)
{
	GL_BeginDebugGroupFormat("R_CopyColor - %s to %s", GL_GetFrameBufferName(src), GL_GetFrameBufferName(dst));

	GL_BindFrameBuffer(dst);

	GL_Set2DEx(0, 0, dst->iWidth, dst->iHeight);

	GL_UseProgram(copy_color.program);

	GL_BindVAO(r_empty_vao);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, src->s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_BindVAO(0);

	GL_UseProgram(0);

	GL_Finish2D();

	GL_EndDebugGroup();
}

void R_CopyColorHaloAdd(FBO_Container_t* src, FBO_Container_t* dst)
{
	GL_BeginDebugGroupFormat("R_CopyColorHaloAdd - %s to %s", GL_GetFrameBufferName(src), GL_GetFrameBufferName(dst));

	GL_BindFrameBuffer(dst);

	GL_Set2DEx(0, 0, dst->iWidth, dst->iHeight);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_BeginStencilCompareNotEqual(STENCIL_MASK_NO_GLOW_BLUR, STENCIL_MASK_NO_GLOW_BLUR);

	GL_UseProgram(copy_color_halo_add.program);

	GL_BindVAO(r_empty_vao);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, src->s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_BindVAO(0);

	GL_UseProgram(0);

	GL_EndStencil();

	glDisable(GL_BLEND);

	GL_Finish2D();

	GL_EndDebugGroup();
}

void R_ShutdownHUD(void)
{
	if (g_TexturedRectVertexBuffer)
	{
		g_TexturedRectVertexBuffer->Destroy();
		g_TexturedRectVertexBuffer = nullptr;
	}
	if (g_FilledRectVertexBuffer)
	{
		g_FilledRectVertexBuffer->Destroy();
		g_FilledRectVertexBuffer = nullptr;
	}
	if (g_RectInstanceBuffer)
	{
		g_RectInstanceBuffer->Destroy();
		g_RectInstanceBuffer = nullptr;
	}
	if (g_RectIndexBuffer)
	{
		g_RectIndexBuffer->Destroy();
		g_RectIndexBuffer = nullptr;
	}
}