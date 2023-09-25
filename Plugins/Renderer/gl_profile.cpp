#include "gl_local.h"

std::vector<glprofile_t *> g_ProfileTable;

glprofile_t Profile_DoHDR;
glprofile_t Profile_GammaCorrection;
glprofile_t Profile_AmbientOcclusion;
glprofile_t Profile_BlendFinalBuffer;
glprofile_t Profile_EndRenderGBuffer;
glprofile_t Profile_DrawEntitiesOnList;
glprofile_t Profile_DrawTransEntities;
glprofile_t Profile_RenderScene;
glprofile_t Profile_RenderShadowScene;
glprofile_t Profile_RenderScene_WaterPass;
glprofile_t Profile_RenderWaterPass;

GLuint GL_GenQuery(void)
{
	GLuint queries[1];
	glGenQueries(1, queries);
	return queries[0];
}

void GL_InitProfile(glprofile_t *profile, const char *name)
{
	profile->query_object_begin = GL_GenQuery();
	profile->query_object_end = GL_GenQuery();
	strncpy(profile->name, name, 63);
	profile->name[63] = 0;
	profile->started = false;

	g_ProfileTable.emplace_back(profile);
}

void GL_InitProfiles(void)
{
	GL_InitProfile(&Profile_RenderWaterPass, "R_RenderWaterPass");
	GL_InitProfile(&Profile_RenderScene_WaterPass, "R_RenderScene_WaterPass");
	GL_InitProfile(&Profile_RenderShadowScene, "R_RenderShadowScene");
	GL_InitProfile(&Profile_RenderScene, "R_RenderScene");
	GL_InitProfile(&Profile_DrawEntitiesOnList, "R_DrawEntitiesOnList");
	GL_InitProfile(&Profile_DrawTransEntities, "R_DrawTransEntities");
	GL_InitProfile(&Profile_EndRenderGBuffer, "R_EndRenderGBuffer");
	GL_InitProfile(&Profile_BlendFinalBuffer, "R_BlendFinalBuffer");
	GL_InitProfile(&Profile_AmbientOcclusion, "R_AmbientOcclusion");
	GL_InitProfile(&Profile_GammaCorrection, "R_GammaCorrection");
	GL_InitProfile(&Profile_DoHDR, "R_DoHDR");
}

void GL_FreeProfiles(void)
{
	for (size_t i = 0; i < g_ProfileTable.size(); ++i)
	{
		glDeleteQueries(1, &g_ProfileTable[i]->query_object_begin);
		glDeleteQueries(1, &g_ProfileTable[i]->query_object_end);
	}

	g_ProfileTable.clear();
}

void GL_BeginProfile(glprofile_t *profile)
{
	if (!gl_profile->value)
		return;

	if (!profile->query_object_begin)
		return;

	if (profile->started)
		return;

	glQueryCounter(profile->query_object_begin, GL_TIMESTAMP);

	profile->cpu_begin = gEngfuncs.GetAbsoluteTime();

	profile->started = true;
}

void GL_EndProfile(glprofile_t *profile)
{
	if (!gl_profile->value)
		return;

	if (!profile->query_object_end)
		return;

	if (!profile->started)
		return;

	profile->cpu_end = gEngfuncs.GetAbsoluteTime();

	glQueryCounter(profile->query_object_end, GL_TIMESTAMP);
}

void GL_PrintProfile(glprofile_t *profile)
{
	if (!gl_profile->value)
		return;

	if (!profile->query_object_begin)
		return;


	if (!profile->query_object_end)
		return;

	if (!profile->started)
		return;

	GLint available = 0;
	GLint available2 = 0;
	while (!available || !available2) {
		glGetQueryObjectiv(profile->query_object_begin, GL_QUERY_RESULT_AVAILABLE, &available);
		glGetQueryObjectiv(profile->query_object_end, GL_QUERY_RESULT_AVAILABLE, &available2);
	}

	if (available && available2)
	{
		double flCPUTime = (profile->cpu_end * 1000.0 * 1000.0 - profile->cpu_begin * 1000.0 * 1000.0);//seconds -> micro seconds

		GLuint64 ulGPUTimeBegin64;
		glGetQueryObjectui64v(profile->query_object_begin, GL_QUERY_RESULT, &ulGPUTimeBegin64);

		GLuint64 ulGPUTimeEnd64;
		glGetQueryObjectui64v(profile->query_object_end, GL_QUERY_RESULT, &ulGPUTimeEnd64);

		double flGPUTime = double(ulGPUTimeEnd64 - ulGPUTimeBegin64) / 1000.0;//nano seconds -> micro seconds

		gEngfuncs.Con_Printf("%s CPU time: %.02f (microsecs), GPU time: %.02f (microsecs)\n", profile->name, flCPUTime, flGPUTime);
	}
}

void GL_Profiles_StartFrame(void)
{
	for (size_t i = 0; i < g_ProfileTable.size(); ++i)
	{
		g_ProfileTable[i]->started = false;
	}
}

void GL_Profiles_EndFrame(void)
{
	for (size_t i = 0; i < g_ProfileTable.size(); ++i)
	{
		if (g_ProfileTable[i]->started)
		{
			GL_PrintProfile(g_ProfileTable[i]);
		}
	}
}