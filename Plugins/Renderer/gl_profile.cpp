#include "gl_local.h"

std::vector<glprofile_t *> g_ProfileTable;

GLuint GL_GenQuery(void)
{
	GLuint queries[1];
	glGenQueries(1, queries);
	return queries[0];
}

void GL_InitProfile(glprofile_t *profile, const char *name)
{
	if (!profile->query_begin)
	{
		profile->query_begin = GL_GenQuery();
		profile->query_end = GL_GenQuery();
		strncpy(profile->name, name, 63);
		profile->name[63] = 0;
		profile->query_begin_started = false;
		profile->query_end_started = false;

		g_ProfileTable.emplace_back(profile);
	}
}

void GL_FreeProfiles(void)
{
	for (size_t i = 0; i < g_ProfileTable.size(); ++i)
	{
		glDeleteQueries(1, &g_ProfileTable[i]->query_begin);
		glDeleteQueries(1, &g_ProfileTable[i]->query_end);
	}

	g_ProfileTable.clear();
}

void GL_BeginProfile(glprofile_t *profile, const char *name)
{
	if (!gl_profile->value)
		return;

	GL_InitProfile(profile, name);

	GLint available = 0;
	glGetQueryObjectiv(profile->query_end, GL_QUERY_RESULT_AVAILABLE, &available);

	if (available)
	{
		GLuint64 beginTime;
		GLuint64 endTime;
		glGetQueryObjectui64v(profile->query_begin, GL_QUERY_RESULT, &beginTime);
		glGetQueryObjectui64v(profile->query_end, GL_QUERY_RESULT, &endTime);

		auto gpuTime = double(endTime - beginTime) / 1000.0;//nano seconds -> micro seconds
		auto cpuTime = (profile->cpu_end * 1000.0 * 1000.0 - profile->cpu_begin * 1000.0 * 1000.0);//seconds -> micro seconds

		gEngfuncs.Con_Printf("%s CPU time: %.02f (microsecs), GPU time: %.02f (microsecs)\n", profile->name, cpuTime, gpuTime);

		profile->query_begin_started = false;
		profile->query_end_started = false;
	}

	if (!profile->query_begin_started)
	{
		glQueryCounter(profile->query_begin, GL_TIMESTAMP);
		profile->query_begin_started = true;
	}

	profile->cpu_begin = gEngfuncs.GetAbsoluteTime();
}

void GL_EndProfile(glprofile_t *profile)
{
	if (!gl_profile->value)
		return;

	profile->cpu_end = gEngfuncs.GetAbsoluteTime();

	if (!profile->query_end_started)
	{
		glQueryCounter(profile->query_end, GL_TIMESTAMP);
		profile->query_end_started = true;
	}
}