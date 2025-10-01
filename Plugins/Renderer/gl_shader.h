#pragma once

#include <vector>
#include <unordered_map>

typedef uint64_t program_state_t;

typedef struct program_state_mapping_s
{
	program_state_t state;
	const char *name;
}program_state_mapping_t;

class CCompileShaderArgs
{
public:
	const char *vsfile{};
	const char *gsfile{};
	const char *fsfile{};
	const char *vsdefine{};
	const char *gsdefine{};
	const char *fsdefine{};
};

class CCompileShaderContext
{
public:
	std::string vscode;
	std::string gscode;
	std::string fscode;
};

GLuint R_CompileShaderFile(const char* vsfile, const char* fsfile, const char* vsdefine = nullptr, const char* fsdefine = nullptr);
GLuint R_CompileShaderFileEx(const CCompileShaderArgs *args);

void GL_UseProgram(GLuint program);
GLuint GL_GetUniformLoc(GLuint program, const char *name);
GLuint GL_GetAttribLoc(GLuint program, const char *name);
void GL_Uniform1i(GLuint loc, int v0);
void GL_Uniform2i(GLuint loc, int v0, int v1);
void GL_Uniform3i(GLuint loc, int v0, int v1, int v2);
void GL_Uniform4i(GLuint loc, int v0, int v1, int v2, int v3);
void GL_Uniform1f(GLuint loc, float v0);
void GL_Uniform2f(GLuint loc, float v0, float v1);
void GL_Uniform3f(GLuint loc, float v0, float v1, float v2);
void GL_Uniform4f(GLuint loc, float v0, int v1, int v2, int v3);

void R_LoadProgramStateCaches(const char *filename, const program_state_mapping_t *mapping, size_t mapping_size, void(*callback)(program_state_t state));

void R_SaveProgramStatesCaches(const char *filename, const std::vector<program_state_t> &ProgramStates, const program_state_mapping_t *mapping, size_t mapping_size);

#define SHADER_DEFINE(name) name##_program_t name;

#define SHADER_UNIFORM(name, loc, locstring) name##.loc = glGetUniformLocation(name.program, locstring);
#define SHADER_ATTRIB(name, loc, locstring) name##.loc = glGetAttribLocation(name.program, locstring);
