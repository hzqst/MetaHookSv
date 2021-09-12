#include "gl_local.h"
#include "gl_shader.h"
#include <string>
#include <sstream>
#include <regex>

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

void GL_CheckShaderError(GLuint shader, const char *code, const char *filename)
{
	int iStatus;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &iStatus); 

	if(!iStatus)
	{
		int nInfoLength;
		char szCompilerLog[1024] = { 0 };
		glGetInfoLogARB(shader, sizeof(szCompilerLog) - 1, &nInfoLength, szCompilerLog);
		szCompilerLog[nInfoLength] = 0;

		g_pFileSystem->CreateDirHierarchy("logs");
		g_pFileSystem->CreateDirHierarchy("logs/renderer");
		auto FileHandle = g_pFileSystem->Open("logs/renderer/error.log", "wb");
		if (FileHandle)
		{
			g_pFileSystem->Write(code, strlen(code), FileHandle);
			g_pFileSystem->Write("\n\nFilename: ", sizeof("\n\nFilename: ") - 1, FileHandle);
			g_pFileSystem->Write(filename, strlen(filename), FileHandle);
			g_pFileSystem->Write("\n\nLogs: ", sizeof("\n\nLogs: ") - 1, FileHandle);
			g_pFileSystem->Write(szCompilerLog, nInfoLength, FileHandle);
			g_pFileSystem->Close(FileHandle);
		}
		Sys_ErrorEx("Shader %s compiled with error:\n%s", filename, szCompilerLog);
		return;
	}
}

GLuint R_CompileShaderObject(int type, const char *code, const char *filename)
{
	auto obj = glCreateShaderObjectARB(type);

	glShaderSource(obj, 1, &code, NULL);

	glCompileShader(obj);

	GL_CheckShaderError(obj, code, filename);

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

void R_CompileShaderAppendInclude(std::string &str, const char *filename)
{
	std::regex pattern("#include[< \"]+([a-zA-Z_\\.]+)[> \"]");
	std::smatch result;
	std::regex_search(str, result, pattern);

	std::string skipped;

	std::string::const_iterator searchStart(str.cbegin());

	while (std::regex_search(searchStart, str.cend(), result, pattern) && result.size() >= 2)
	{
		std::string prefix = result.prefix();
		std::string suffix = result.suffix();

		auto includeFileName = result[1].str();

		char slash;

		std::string includePath = filename;
		for (size_t j = includePath.length() - 1; j > 0; --j)
		{
			if (includePath[j] == '\\' || includePath[j] == '/')
			{
				slash = includePath[j];
				includePath.resize(j);
				break;
			}
		}

		includePath += slash;
		includePath += includeFileName;

		auto pFile = gEngfuncs.COM_LoadFile((char *)includePath.c_str(), 5, NULL);
		if (pFile)
		{
			std::string wbinding((char *)pFile);

			gEngfuncs.COM_FreeFile(pFile);

			if (searchStart != str.cbegin())
			{
				str = skipped + prefix;
			}
			else
			{
				str = prefix;
			}
			str += wbinding;

			auto currentLength = str.length();

			str += suffix;

			skipped = str.substr(0, currentLength);
			searchStart = str.cbegin() + currentLength;
			continue;
		}

		searchStart = result.suffix().first;
	}
}

void R_CompileShaderAppendDefine(std::string &str, const std::string &def)
{
	std::regex pattern("(#version [0-9a-z ]+)");
	std::smatch result;
	std::regex_search(str, result, pattern);

	if (result.size() >= 1)
	{
		std::string prefix = result[0];
		std::string suffix = result.suffix();

		str = prefix;
		str += "\n\n";
		str += def;
		str += "\n\n";
		str += suffix;
	}
	else
	{
		std::string suffix = str;

		str = def;
		str += "\n\n";
		str += suffix;
	}
}

GLuint R_CompileShaderFileEx(
	const char *vsfile, const char *fsfile, 
	const char *vsdefine, const char *fsdefine,
	ExtraShaderStageCallback callback)
{
	auto vscode = (char *)gEngfuncs.COM_LoadFile((char *)vsfile, 5, 0);
	if (!vscode)
	{
		Sys_ErrorEx("R_CompileShaderFileEx: %s not found!", vsfile);
	}

	std::string vs(vscode);

	R_CompileShaderAppendDefine(vs, "#define IS_VERTEX_SHADER\n");
	if (vsdefine)
	{
		R_CompileShaderAppendDefine(vs, vsdefine);
	}

	gEngfuncs.COM_FreeFile(vscode);

	auto fscode = (char *)gEngfuncs.COM_LoadFile((char *)fsfile, 5, 0);
	if (!fscode)
	{
		Sys_ErrorEx("R_CompileShaderFileEx: %s not found!", fsfile);
	}

	std::string fs(fscode);

	R_CompileShaderAppendDefine(fs, "#define IS_FRAGMENT_SHADER\n");
	if (fsdefine)
	{
		R_CompileShaderAppendDefine(fs, fsdefine);
	}

	gEngfuncs.COM_FreeFile(fscode);

	if (vs.find("#include") != std::string::npos)
	{
		R_CompileShaderAppendInclude(vs, vsfile);
	}

	if (fs.find("#include") != std::string::npos)
	{
		R_CompileShaderAppendInclude(fs, fsfile);
	}

	return R_CompileShader(vs.c_str(), fs.c_str(), vsfile, fsfile, callback);
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
	glUniform2f(loc, v0, v1);
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