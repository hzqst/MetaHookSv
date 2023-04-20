#pragma once

#include <vector>
#include <unordered_map>

typedef struct glshader_s
{
	struct glshader_s(GLuint prog, GLuint *objs, int used)
	{
		program = prog;
		shader_objects.resize(used);
		memcpy(shader_objects.data(), objs, used * sizeof(GLuint));
	}
	GLuint program;
	std::vector<GLuint> shader_objects;
}glshader_t;

typedef uint64_t program_state_t;

typedef struct program_state_mapping_s
{
	program_state_t state;
	const char *name;
}program_state_mapping_t;

typedef void(*ExtraShaderStageCallback)(GLuint *objs, int *used);
GLuint R_CompileShaderObject(int type, const char *code, const char *filename);
GLuint R_CompileShader(const char *vscode, const char *fscode, const char *vsfile, const char *fsfile, ExtraShaderStageCallback callback);
GLuint R_CompileShaderFile(const char *vsfile, const char *fsfile, ExtraShaderStageCallback callback);
GLuint R_CompileShaderFileEx(const char *vsfile, const char *fsfile, const char *vsdefine, const char *fsdefine, ExtraShaderStageCallback callback);
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
void GL_VertexAttrib3f(GLuint index, float x, float y, float z);
void GL_VertexAttrib3fv(GLuint index, float *v);
void GL_MultiTexCoord2f(GLenum target, float s, float t);
void GL_MultiTexCoord3f(GLenum target, float s, float t, float r);

void R_LoadProgramStateCaches(const char *filename, const program_state_mapping_t *mapping, size_t mapping_size, void(*callback)(program_state_t state));

void R_SaveProgramStatesCaches(const char *filename, const std::vector<program_state_t> &ProgramStates, const program_state_mapping_t *mapping, size_t mapping_size);

#define clamp(value, mi, ma) min(max(value, mi), ma)

#define SHADER_DEFINE(name) name##_program_t name;

#define SHADER_UNIFORM(name, loc, locstring) name##.loc = glGetUniformLocationARB(name.program, locstring);
#define SHADER_ATTRIB(name, loc, locstring) name##.loc = glGetAttribLocationARB(name.program, locstring);
