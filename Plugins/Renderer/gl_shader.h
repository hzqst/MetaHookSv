#pragma once

#include <vector>
#include <unordered_map>

typedef uint64_t program_state_t;

typedef struct program_state_mapping_s
{
	program_state_t state;
	const char *name;
}program_state_mapping_t;

GLuint GL_CompileShaderFile(const char* vsfile, const char* fsfile, const char* vsdefine = nullptr, const char* fsdefine = nullptr);
GLuint GL_CompileShaderFileEx(const CCompileShaderArgs *args);

void GL_UseProgram(GLuint program);

void R_LoadProgramStateCaches(const char *filename, const program_state_mapping_t *mapping, size_t mapping_size, void(*callback)(program_state_t state));

void R_SaveProgramStatesCaches(const char *filename, const std::vector<program_state_t> &ProgramStates, const program_state_mapping_t *mapping, size_t mapping_size);

#define SHADER_DEFINE(name) name##_program_t name;

#define SHADER_UNIFORM(name, loc, locstring) name##.loc = glGetUniformLocation(name.program, locstring);
#define SHADER_ATTRIB(name, loc, locstring) name##.loc = glGetAttribLocation(name.program, locstring);
