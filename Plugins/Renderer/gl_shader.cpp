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
		auto &objs = g_ShaderTable[i].shader_objects;
		for (size_t j = 0; j < objs.size(); ++j)
		{
			glDetachObjectARB(g_ShaderTable[i].program, objs[j]);
			glDeleteObjectARB(objs[j]);
		}
		glDeleteProgramsARB(1, &g_ShaderTable[i].program);
	}

	g_ShaderTable.clear();
}

void GL_CheckShaderError(GLuint shader, const char *filename)
{
	int iStatus;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &iStatus); 

	if(!iStatus)
	{
		int nInfoLength;
		char szCompilerLog[1024] = { 0 };
		glGetInfoLogARB(shader, sizeof(szCompilerLog) - 1, &nInfoLength, szCompilerLog);
		szCompilerLog[nInfoLength] = 0;

		Sys_ErrorEx("Shader %s compiled with error:\n%s", filename, szCompilerLog);
		return;
	}
}

GLuint R_CompileShaderObject(int type, const char *code, const char *filename)
{
	auto obj = glCreateShaderObjectARB(type);
	glShaderSourceARB(obj, 1, &code, NULL);
	glCompileShaderARB(obj);

	GL_CheckShaderError(obj, filename);

	return obj;
}

GLuint R_CompileShader(const char *vscode, const char *fscode, const char *vsfile, const char *fsfile, ExtraShaderStageCallback callback)
{
	GLuint shader_objects[32];
	int shader_object_used = 0;

	shader_objects[shader_object_used] = R_CompileShaderObject(GL_VERTEX_SHADER_ARB, vscode, vsfile);
	shader_object_used++;

	if(callback)
		callback(shader_objects, &shader_object_used);

	shader_objects[shader_object_used] = R_CompileShaderObject(GL_FRAGMENT_SHADER_ARB, fscode, fsfile);
	shader_object_used++;

	GLuint program = glCreateProgramObjectARB();
	for(int i = 0;i < shader_object_used; ++i)
		glAttachObjectARB(program, shader_objects[i]);
	glLinkProgramARB(program);

	int iStatus;
	glGetProgramiv(program, GL_LINK_STATUS, &iStatus);
	if (!iStatus)
	{
		int nInfoLength;
		char szCompilerLog[1024] = { 0 };
		glGetProgramInfoLog(program, sizeof(szCompilerLog), &nInfoLength, szCompilerLog);

		Sys_ErrorEx("Shader linked with error:\n%s", szCompilerLog);
	}

	g_ShaderTable.emplace_back(program, shader_objects, shader_object_used);

	return program;
}

GLuint R_CompileShaderFileEx(
	const char *vsfile, const char *fsfile, 
	const char *vsdefine, const char *fsdefine,
	ExtraShaderStageCallback callback)
{
	char *vscode = NULL;
	char *fscode = NULL;
	
	std::stringstream vss, gss, fss;

	vscode = (char *)gEngfuncs.COM_LoadFile((char *)vsfile, 5, 0);
	if (!vscode)
	{
		Sys_ErrorEx("R_CompileShaderFileEx: %s not found!", vsfile);
	}

	fscode = (char *)gEngfuncs.COM_LoadFile((char *)fsfile, 5, 0);
	if (!fscode)
	{
		Sys_ErrorEx("R_CompileShaderFileEx: %s not found!", fsfile);
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

	return R_CompileShader(vss.str().c_str(), fss.str().c_str(), vsfile, fsfile, callback);
}

GLuint R_CompileShaderFile(const char *vsfile, const char *fsfile, ExtraShaderStageCallback callback)
{
	return R_CompileShaderFileEx(vsfile, fsfile, NULL, NULL, callback);
}

void GL_UseProgram(GLuint program)
{
	static int currentprogram = -1;

	if (currentprogram != program)
	{
		currentprogram = program;
		glUseProgramObjectARB(program);
	}
}

GLuint GL_GetUniformLoc(GLuint program, const char *name)
{
	return glGetUniformLocationARB(program, name);
}

GLuint GL_GetAttribLoc(GLuint program, const char *name)
{
	return glGetAttribLocationARB(program, name);
}

void GL_Uniform1i(GLuint loc, int v0)
{
	glUniform1i(loc, v0);
}

void GL_Uniform2i(GLuint loc, int v0, int v1)
{
	glUniform2iARB(loc, v0, v1);
}

void GL_Uniform3i(GLuint loc, int v0, int v1, int v2)
{
	glUniform3iARB(loc, v0, v1, v2);
}

void GL_Uniform4i(GLuint loc, int v0, int v1, int v2, int v3)
{
	glUniform4iARB(loc, v0, v1, v2, v3);
}

void GL_Uniform1f(GLuint loc, float v0)
{
	glUniform1f(loc, v0);
}

void GL_Uniform2f(GLuint loc, float v0, float v1)
{
	glUniform2fARB(loc, v0, v1);
}

void GL_Uniform3f(GLuint loc, float v0, float v1, float v2)
{
	glUniform3f(loc, v0, v1, v2);
}

void GL_Uniform4f(GLuint loc, float v0, int v1, int v2, int v3)
{
	glUniform4f(loc, v0, v1, v2, v3);
}

void GL_VertexAttrib3f(GLuint index, float x, float y, float z)
{
	glVertexAttrib3f(index, x, y, z);
}

void GL_VertexAttrib3fv(GLuint index, float *v)
{
	glVertexAttrib3fv(index, v);
}

void GL_MultiTexCoord2f(GLenum target, float s, float t)
{
	glMultiTexCoord2fARB(target, s, t);
}

void GL_MultiTexCoord3f(GLenum target, float s, float t, float r)
{
	glMultiTexCoord3fARB(target, s, t, r);
}