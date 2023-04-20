#pragma once

typedef struct glprofile_s
{
	char name[64];
	GLuint query_object_begin;
	GLuint query_object_end;
	double cpu_begin;
	double cpu_end;
	bool started;
}glprofile_t;

extern glprofile_t Profile_DoHDR;
extern glprofile_t Profile_DoFXAA;
extern glprofile_t Profile_GammaCorrection;
extern glprofile_t Profile_AmbientOcclusion;
extern glprofile_t Profile_BlendFinalBuffer;
extern glprofile_t Profile_EndRenderGBuffer;
extern glprofile_t Profile_DrawTransEntities;
extern glprofile_t Profile_DrawEntitiesOnList;
extern glprofile_t Profile_RenderScene;
extern glprofile_t Profile_RenderShadowScene;
extern glprofile_t Profile_RenderScene_WaterPass;
extern glprofile_t Profile_RenderWaterPass;

extern cvar_t *gl_profile;

void GL_InitProfiles(void);
void GL_FreeProfiles(void);
void GL_BeginProfile(glprofile_t *profile);
void GL_EndProfile(glprofile_t *profile);
void GL_PrintProfile(glprofile_t *profile);
void GL_Profiles_StartFrame(void);
void GL_Profiles_EndFrame(void);