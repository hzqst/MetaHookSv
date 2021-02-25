#include "gl_local.h"
#include "gl_shader.h"
#include <string>
#include <sstream>

std::vector<glshader_t> g_ShaderTable;

void GL_InitShaders(void)
{
	
}

void GL_FreeShaders(void)
{
	for(size_t i = 0;i < g_ShaderTable.size(); ++i)
	{
		if (g_ShaderTable[i].vs)
		{
			qglDetachObjectARB(g_ShaderTable[i].program, g_ShaderTable[i].vs);
			qglDeleteObjectARB(g_ShaderTable[i].vs);
		}
		if (g_ShaderTable[i].gs)
		{
			qglDetachObjectARB(g_ShaderTable[i].program, g_ShaderTable[i].gs);
			qglDeleteObjectARB(g_ShaderTable[i].gs);
		}
		if (g_ShaderTable[i].fs)
		{
			qglDetachObjectARB(g_ShaderTable[i].program, g_ShaderTable[i].fs);
			qglDeleteObjectARB(g_ShaderTable[i].fs);
		}
		qglDeleteProgramsARB(1, &g_ShaderTable[i].program);
	}

	g_ShaderTable.clear();
}

void GL_CheckShaderError(GLuint shader, const char *filename)
{
	int iStatus;

	qglGetShaderiv(shader, GL_COMPILE_STATUS, &iStatus); 

	if(!iStatus)
	{
		int nInfoLength;
		char szCompilerLog[1024] = { 0 };
		qglGetInfoLogARB(shader, sizeof(szCompilerLog), &nInfoLength, szCompilerLog);

		Sys_ErrorEx("Shader %s compiled with error:\n%s", filename, szCompilerLog);
		return;
	}
}

GLuint R_CompileShader(const char *vscode, const char *gscode, const char *fscode, const char *vsfile, const char *gsfile, const char *fsfile)
{
	GLuint vs = 0, gs = 0, fs = 0;

	if (vscode && vscode[0])
	{
		vs = qglCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
		qglShaderSourceARB(vs, 1, &vscode, NULL);
		qglCompileShaderARB(vs);
		GL_CheckShaderError(vs, vsfile);
	}

	if (gscode && gscode[0])
	{
		gs = qglCreateShaderObjectARB(GL_GEOMETRY_SHADER_ARB);
		qglShaderSourceARB(gs, 1, &gscode, NULL);
		qglCompileShaderARB(gs);
		GL_CheckShaderError(gs, gsfile);
	}

	fs = qglCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	qglShaderSourceARB(fs, 1, &fscode, NULL);
	qglCompileShaderARB(fs);
	GL_CheckShaderError(fs, fsfile);

	GLuint program = qglCreateProgramObjectARB();
	if (vs)
	{
		qglAttachObjectARB(program, vs);
	}
	if (gs)
	{
		qglAttachObjectARB(program, gs);
	}
	qglAttachObjectARB(program, fs);
	qglLinkProgramARB(program);

	/*int iStatus;
	qglGetProgramiv(program, GL_LINK_STATUS, &iStatus);
	if (!iStatus)
	{
		int nInfoLength;
		char szCompilerLog[1024] = { 0 };
		qglGetProgramInfoLog(program, sizeof(szCompilerLog), &nInfoLength, szCompilerLog);

		Sys_ErrorEx("Shader linked with error:\n%s", szCompilerLog);
		return 0;
	}*/

	g_ShaderTable.emplace_back(vs, gs, fs, program);

	return program;
}

GLuint R_CompileShaderFileEx(
	const char *vsfile, const char *gsfile, const char *fsfile, 
	const char *vsdefine, const char *gsdefine, const char *fsdefine)
{
	char *vscode = NULL;
	char *gscode = NULL;
	char *fscode = NULL;
	
	std::stringstream vss, gss, fss;

	vscode = (char *)gEngfuncs.COM_LoadFile((char *)vsfile, 5, 0);
	if (!vscode)
	{
		Sys_ErrorEx("R_CompileShaderFileEx: %s not found!", vsfile);
	}

	if (gsfile)
	{
		gscode = (char *)gEngfuncs.COM_LoadFile((char *)gsfile, 5, 0);
		if (!fscode)
		{
			Sys_ErrorEx("R_CompileShaderFileEx: %s not found!", vsfile);
		}
	}

	fscode = (char *)gEngfuncs.COM_LoadFile((char *)fsfile, 5, 0);
	if (!fscode)
	{
		Sys_ErrorEx("R_CompileShaderFileEx: %s not found!", vsfile);
	}

	if (vscode && vsdefine)
	{
		if (!strncmp(vscode, "#version ", sizeof("#version ") - 1))
		{
			char *pcode = vscode + sizeof("#version ") - 1;
			while (*pcode && (isdigit(*pcode) ||  *pcode == ' ' || *pcode == '\r' || *pcode == '\n'))
			{
				pcode++;
			}
			vss << std::string(vscode, pcode - vscode);
			vss << vsdefine << "\n";
			vss << pcode;
		}
		else
		{
			vss << vsdefine << "\n";
			vss << vscode;
		}
	}
	else if (vscode)
	{
		vss << vscode;
	}

	if (gscode && gsdefine)
	{
		if (!strncmp(gscode, "#version ", sizeof("#version ") - 1))
		{
			char *pcode = gscode + sizeof("#version ") - 1;
			while (*pcode && (isdigit(*pcode) || *pcode == ' ' || *pcode == '\r' || *pcode == '\n'))
			{
				pcode++;
			}
			gss << std::string(gscode, pcode - gscode);
			gss << gsdefine << "\n";
			gss << pcode;
		}
		else
		{
			gss << gsdefine << "\n";
			gss << gscode;
		}
	}
	else if (gscode)
	{
		gss << gscode;
	}

	if (fscode && fsdefine)
	{
		if (!strncmp(fscode, "#version ", sizeof("#version ") - 1))
		{
			char *pcode = fscode + sizeof("#version ") - 1;
			while (*pcode && (isdigit(*pcode) || *pcode == ' ' || *pcode == '\r' || *pcode == '\n'))
			{
				pcode++;
			}
			fss << std::string(fscode, pcode - fscode);
			fss << fsdefine << "\n";
			fss << pcode;
		}
		else
		{
			fss << fsdefine << "\n";
			fss << fscode;
		}
	}
	else if (fscode)
	{
		fss << fscode;
	}

	auto r = R_CompileShader(vss.str().c_str(), gss.str().c_str(), fss.str().c_str(), vsfile, gsfile, fsfile);

	return r;
}


GLuint R_CompileShaderFile(const char *vsfile, const char *gsfile, const char *fsfile)
{
	return R_CompileShaderFileEx(vsfile, gsfile, fsfile, NULL, NULL, NULL);
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