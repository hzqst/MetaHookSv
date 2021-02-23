#include "gl_local.h"
#include "gl_shader.h"

glshader_t shaders[MAX_SHADERS];
int numshaders;

void GL_InitShaders(void)
{
	numshaders = 0;
	memset(shaders, 0, sizeof(shaders));
}

void GL_FreeShaders(void)
{
	for (int i = 0; i < numshaders; i++)
	{
		if(shaders[i].program)
		{
			qglDetachObjectARB(shaders[i].program, shaders[i].vs);
			qglDetachObjectARB(shaders[i].program, shaders[i].fs);
			qglDeleteObjectARB(shaders[i].vs);
			qglDeleteObjectARB(shaders[i].fs);
			qglDeleteProgramsARB(1, (GLuint *)&shaders[i].program);
		}
	}
	numshaders = 0;
}

void GL_CheckShaderError(GLuint shader, const char *filename)
{
	char szCompilerLog[1024];
	int iStatus, nInfoLength;

	qglGetShaderiv(shader, GL_COMPILE_STATUS, &iStatus); 
	qglGetInfoLogARB(shader, sizeof(szCompilerLog), &nInfoLength, szCompilerLog);

	if(!iStatus)
	{
		Sys_ErrorEx("Shader %s compiled with error:\n%s", filename, szCompilerLog);
		return;
	}
}

GLuint R_CompileShader(const char *vscode, const char *gscode, const char *fscode, const char *vsfile, const char *gsfile, const char *fsfile)
{
	if (numshaders + 1 == MAX_SHADERS)
	{
		Sys_ErrorEx("R_CompileShader: MAX_SHADERS exceeded");
		return 0;
	}

	GLuint vs = 0, gs = 0, fs = 0;

	vs = qglCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	qglShaderSourceARB(vs, 1, &vscode, NULL);
	qglCompileShaderARB(vs);
	GL_CheckShaderError(vs, vsfile);

	if (gscode != NULL)
	{
		gs = qglCreateShaderObjectARB(GL_GEOMETRY_SHADER_ARB);
		qglShaderSourceARB(gs, 1, &gscode, NULL);
		qglCompileShaderARB(gs);
		GL_CheckShaderError(vs, gsfile);
	}

	fs = qglCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	qglShaderSourceARB(fs, 1, &fscode, NULL);
	qglCompileShaderARB(fs);
	GL_CheckShaderError(vs, fsfile);

	GLuint program = qglCreateProgramObjectARB();
	qglAttachObjectARB(program, vs);
	if (gs)
	{
		qglAttachObjectARB(program, gs);
	}
	qglAttachObjectARB(program, fs);
	qglLinkProgramARB(program);

	shaders[numshaders].program = program;
	shaders[numshaders].vs = vs;
	shaders[numshaders].gs = gs;
	shaders[numshaders].fs = fs;
	numshaders ++;

	return program;
}

GLuint R_CompileShaderFile(const char *vsfile, const char *gsfile, const char *fsfile)
{
	char *vscode = NULL;
	char *gscode = NULL;
	char *fscode = NULL;

	vscode = (char *)gEngfuncs.COM_LoadFile((char *)vsfile, 5, 0);
	if (!vscode)
	{
		Sys_ErrorEx("R_CompileShaderFile: %s not found!", vsfile);
	}

	if (gsfile)
	{
		gscode = (char *)gEngfuncs.COM_LoadFile((char *)gsfile, 5, 0);
		if (!fscode)
		{
			Sys_ErrorEx("R_CompileShaderFile: %s not found!", vsfile);
		}
	}

	fscode = (char *)gEngfuncs.COM_LoadFile((char *)fsfile, 5, 0);
	if (!fscode)
	{
		Sys_ErrorEx("R_CompileShaderFile: %s not found!", vsfile);
	}

	auto program = R_CompileShader(vscode, gscode, fscode, vsfile, gsfile, fsfile);
	
	if (vscode)
		gEngfuncs.COM_FreeFile(vscode);
	if (gscode)
		gEngfuncs.COM_FreeFile(gscode);
	if (fscode)
		gEngfuncs.COM_FreeFile(fscode);

	return program;
}

GLuint R_CompileShaderFileEx(
	const char *vsfile, const char *gsfile, const char *fsfile, 
	const char *vsdefine, const char *gsdefine, const char *fsdefine)
{
	char *vscode = NULL;
	char *gscode = NULL;
	char *fscode = NULL;
	char *vs = NULL;
	char *gs = NULL;
	char *fs = NULL;

	vscode = (char *)gEngfuncs.COM_LoadFile((char *)vsfile, 5, 0);
	if (!vscode)
	{
		Sys_ErrorEx("R_CompileShaderFile: %s not found!", vsfile);
	}

	if (gsfile)
	{
		gscode = (char *)gEngfuncs.COM_LoadFile((char *)gsfile, 5, 0);
		if (!fscode)
		{
			Sys_ErrorEx("R_CompileShaderFile: %s not found!", vsfile);
		}
	}

	fscode = (char *)gEngfuncs.COM_LoadFile((char *)fsfile, 5, 0);
	if (!fscode)
	{
		Sys_ErrorEx("R_CompileShaderFile: %s not found!", vsfile);
	}

	if (vscode && vsdefine)
	{
		auto vslen = strlen(vscode);
		auto vsdeflen = strlen(vsdefine);
		vs = new char[vslen + 1 + vsdeflen + 1];
		strcpy(vs, vsdefine);
		strcat(vs, "\n");
		strcat(vs, vscode);
	}

	if (gscode && gsdefine)
	{
		auto gslen = strlen(gscode);
		auto gsdeflen = strlen(gsdefine);
		gs = new char[gslen + 1 + gsdeflen + 1];
		strcpy(gs, gsdefine);
		strcat(gs, "\n");
		strcat(gs, gscode);
	}

	if (fscode && fsdefine)
	{
		auto fslen = strlen(fscode);
		auto fsdeflen = strlen(fsdefine);
		fs = new char[fslen + 1 + fsdeflen + 1];
		strcpy(fs, fsdefine);
		strcat(fs, "\n");
		strcat(fs, fscode);
	}

	auto r = R_CompileShader(vs, gs, fs, vsfile, gsfile, fsfile);

	if (vs)
		delete[]vs;
	if (fs)
		delete[]fs;
	if (gs)
		delete[]gs;

	return r;
}

void GL_UseProgram(GLuint program)
{
	qglUseProgramObjectARB(program);
}

void GL_EndProgram(void)
{
	qglUseProgramObjectARB(0);
}

GLuint GL_GetUniformLoc(GLuint program, const char *name)
{
	return qglGetUniformLocationARB(program, name);
}

GLuint GL_GetAttribLoc(GLuint program, const char *name)
{
	return qglGetAttribLocationARB(program, name);
}

void GL_Uniform1i(GLuint loc, int v0)
{
	qglUniform1iARB(loc, v0);
}

void GL_Uniform2i(GLuint loc, int v0, int v1)
{
	qglUniform2iARB(loc, v0, v1);
}

void GL_Uniform3i(GLuint loc, int v0, int v1, int v2)
{
	qglUniform3iARB(loc, v0, v1, v2);
}

void GL_Uniform4i(GLuint loc, int v0, int v1, int v2, int v3)
{
	qglUniform4iARB(loc, v0, v1, v2, v3);
}

void GL_Uniform1f(GLuint loc, float v0)
{
	qglUniform1fARB(loc, v0);
}

void GL_Uniform2f(GLuint loc, float v0, float v1)
{
	qglUniform2fARB(loc, v0, v1);
}

void GL_Uniform3f(GLuint loc, float v0, float v1, float v2)
{
	qglUniform3fARB(loc, v0, v1, v2);
}

void GL_Uniform4f(GLuint loc, float v0, int v1, int v2, int v3)
{
	qglUniform4fARB(loc, v0, v1, v2, v3);
}

void GL_VertexAttrib3f(GLuint index, float x, float y, float z)
{
	qglVertexAttrib3f(index, x, y, z);
}

void GL_VertexAttrib3fv(GLuint index, float *v)
{
	qglVertexAttrib3fv(index, v);
}

void GL_MultiTexCoord2f(GLenum target, float s, float t)
{
	qglMultiTexCoord2fARB(target, s, t);
}

void GL_MultiTexCoord3f(GLenum target, float s, float t, float r)
{
	qglMultiTexCoord3fARB(target, s, t, r);
}