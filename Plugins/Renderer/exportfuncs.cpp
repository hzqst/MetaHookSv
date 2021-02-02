#include <metahook.h>
#include "exportfuncs.h"
#include "gl_local.h"
#include "command.h"
#include "parsemsg.h"
#include "qgl.h"

//Error when can't find sig
void Sys_ErrorEx(const char *fmt, ...)
{
	va_list argptr;
	char msg[1024];

	va_start(argptr, fmt);
	_vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end(argptr);

	if(gEngfuncs.pfnClientCmd)
		gEngfuncs.pfnClientCmd("escape\n");

	MessageBox(NULL, msg, "Error", MB_ICONERROR);
	exit(0);
}

int Q_stricmp_slash(const char *s1, const char *s2)
{
	int c1, c2;

	while (1)
	{
		c1 = *s1++;
		c2 = *s2++;

		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');

			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');

			if (c1 == '\\')
				c1 = '/';

			if (c2 == '\\')
				c2 = '/';

			if (c1 != c2)
				return -1;
		}

		if (!c1)
			return 0;
	}
	return -1;
}

char *UTIL_VarArgs(char *format, ...)
{
	va_list argptr;
	static int index = 0;
	static char string[16][1024];

	va_start(argptr, format);
	vsprintf(string[index], format, argptr);
	va_end(argptr);

	char *result = string[index];
	index = (index + 1) % 16;
	return result;
}

cl_enginefunc_t gEngfuncs;
engine_studio_api_t IEngineStudio;
r_studio_interface_t **gpStudioInterface;

shaderapi_t ShaderAPI = 
{
	R_CompileShader,
	GL_UseProgram,
	GL_EndProgram,
	GL_GetUniformLoc,
	GL_GetAttribLoc,
	GL_Uniform1i,
	GL_Uniform2i,
	GL_Uniform3i,
	GL_Uniform4i,
	GL_Uniform1f,
	GL_Uniform2f,
	GL_Uniform3f,
	GL_Uniform4f,
	GL_VertexAttrib3f,
	GL_VertexAttrib3fv,
	GL_MultiTexCoord2f,
	GL_MultiTexCoord3f
};

engrefapi_t RefAPI = 
{
	GL_Bind,
	GL_SelectTexture,
	GL_DisableMultitexture,
	GL_EnableMultitexture,
	R_DrawBrushModel,
	R_DrawSpriteModel,
	R_GetSpriteAxes,
	R_SpriteColor,
	GlowBlend,
	CL_FxBlend,
	R_CullBox
};

ref_export_t gRefExports =
{
	//common
	R_GetDrawPass,
	R_GetSupportExtension,
	//studio
	R_GLStudioDrawPointsEx,
	R_GetPlayerState,
	//refdef
	R_PushRefDef,
	R_UpdateRefDef,
	R_PopRefDef,
	R_GetSavedViewOrg,
	R_GetRefDef,
	//framebuffer
	R_PushFrameBuffer,
	R_PopFrameBuffer,
	R_GLBindFrameBuffer,
	//shadow
	R_CreateShadowLight,
	//texture
	R_GLGenTexture,
	R_GetTexLoaderBuffer,
	R_LoadTextureEx,
	GL_LoadTextureEx,
	R_GetCurrentGLTexture,
	GL_UploadDXT,
	LoadDDS,
	LoadImageGeneric,
	SaveImageGeneric,
	//capture screen
	R_GetSCRCaptureBuffer,
	//3dsky
	R_Add3DSkyEntity,
	R_Setup3DSkyModel,
	R_Finish3DSkyModel,
	//2d postprocess
	R_BeginFXAA,
	R_BeginDrawRoundRect,
	R_BeginDrawHudMask,
	//cloak
	R_RenderCloakTexture,
	R_GetCloakTexture,
	R_BeginRenderConc,
	//3dhud
	R_Get3DHUDTexture,
	R_Draw3DHUDQuad,
	R_BeginDrawTrianglesInHUD_Direct,
	R_BeginDrawTrianglesInHUD_FBO,
	R_FinishDrawTrianglesInHUD,
	R_BeginDrawHUDInWorld,
	R_FinishDrawHUDInWorld,
	//shader
	ShaderAPI,
	RefAPI
};

HWND GetMainHWND()
{
	//todo
	return 0;
}

void hudGetMousePos(struct tagPOINT *ppt)
{
	gEngfuncs.pfnGetMousePos(ppt);

	if ( !g_bWindowed )
	{
		RECT rectWin;
		GetWindowRect(GetMainHWND(), &rectWin);
		int videoW = g_iVideoWidth;
		int videoH = g_iVideoHeight;
		int winW = rectWin.right - rectWin.left;
		int winH = rectWin.bottom - rectWin.top;
		ppt->x *= (float)videoW / winW;
		ppt->y *= (float)videoH / winH;
		ppt->x *= (*windowvideoaspect - 1) * (ppt->x - videoW / 2);
		ppt->y *= (*videowindowaspect - 1) * (ppt->y - videoH / 2);
	}
}

void hudGetMousePosition(int *x, int *y)
{
	gEngfuncs.GetMousePosition(x, y);

	if ( !g_bWindowed )
	{
		RECT rectWin;
		GetWindowRect(GetMainHWND(), &rectWin);
		int videoW = g_iVideoWidth;
		int videoH = g_iVideoHeight;
		int winW = rectWin.right - rectWin.left;
		int winH = rectWin.bottom - rectWin.top;
		*x *= (float)videoW / winW;
		*y *= (float)videoH / winH;
		*x *= (*windowvideoaspect - 1) * (*x - videoW / 2);
		*y *= (*videowindowaspect - 1) * (*y - videoH / 2);
	}
}

#define METARENDER_SKYCAMERA 1
#define METARENDER_SHADOWMGR 2

int MsgFunc_MetaRender(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int type = READ_BYTE();
	if(type == METARENDER_SHADOWMGR)
	{
		vec3_t angles;
		angles[0] = READ_COORD();
		angles[1] = READ_COORD();
		angles[2] = READ_COORD();
		float radius = READ_COORD();
		float fard = READ_COORD();
		float scale = READ_COORD();
		int texsize = READ_SHORT();
		char *affectmodel = READ_STRING();
		R_CreateShadowManager(affectmodel, angles, radius, fard, scale, texsize);
	}
	
	return 1;
}

void R_Version_f(void)
{
	gEngfuncs.Con_Printf("Renderer Version:\n%s\n", META_RENDERER_VERSION);
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();

	R_Init();

	//gEngfuncs.pfnHookUserMsg("MetaRender", MsgFunc_MetaRender);

	cl_righthand = gEngfuncs.pfnGetCvarPointer("cl_righthand");

	gEngfuncs.pfnAddCommand("r_version", R_Version_f);

	gEngfuncs.pfnConsolePrint("HUD_Init");
}

int HUD_VidInit(void)
{
	R_VidInit();

	return gExportfuncs.HUD_VidInit();
}

void V_CalcRefdef(struct ref_params_s *pparams)
{
	R_CalcRefdef(pparams);

	gExportfuncs.V_CalcRefdef(pparams);
}

void HUD_DrawNormalTriangles(void)
{
	gExportfuncs.HUD_DrawNormalTriangles();
}

void HUD_DrawTransparentTriangles(void)
{
	if (!drawreflect && !drawrefract && !drawshadow)
	{
		R_FreeDeadWaters();
	}

	gExportfuncs.HUD_DrawTransparentTriangles();

	if (!drawreflect && !drawrefract && !drawshadow)
	{
		R_RenderAllShadowScenes();

		for (shadowlight_t *sl = sdlights_active; sl; sl = sl->next)
		{
			sl->free = true;
		}

		for (r_water_t *water = waters_active; water; water = water->next)
		{
			water->free = true;
		}
	}
}

int HUD_Redraw(float time, int intermission)
{
	if(waters_active && r_water_debug->value > 0 && r_water_debug->value <= 3)
	{
		qglDisable(GL_BLEND);
		qglDisable(GL_ALPHA_TEST);
		qglColor4f(1, 1, 1, 1);

		qglEnable(GL_TEXTURE_2D);
		int debugTextureID = 0;
		switch ((int)r_water_debug->value)
		{
		case 1:
			debugTextureID = waters_active->reflectmap;
			break;
		case 2:
			debugTextureID = waters_active->refractmap;
			break;
		case 3:
			debugTextureID = waters_active->depthrefrmap;
			qglUseProgramObjectARB(drawdepth.program);
			break;
		default:
			break;
		}
		if (debugTextureID)
		{
			qglBindTexture(GL_TEXTURE_2D, debugTextureID);
			qglBegin(GL_QUADS);
			qglTexCoord2f(0, 1);
			qglVertex3f(0, 0, 0);
			qglTexCoord2f(1, 1);
			qglVertex3f(glwidth / 2, 0, 0);
			qglTexCoord2f(1, 0);
			qglVertex3f(glwidth / 2, glheight / 2, 0);
			qglTexCoord2f(0, 0);
			qglVertex3f(0, glheight / 2, 0);
			qglEnd();

			if (debugTextureID = waters_active->depthrefrmap)
				qglUseProgramObjectARB(0);
		}
	}
	else if(sdlights_active && r_shadow_debug->value)
	{
		qglDisable(GL_BLEND);
		qglDisable(GL_ALPHA_TEST);
		qglColor4f(1,1,1,1);

		qglEnable(GL_TEXTURE_2D);
		qglBindTexture(GL_TEXTURE_2D, sdlights_active->depthmap);

		qglBegin(GL_QUADS);
		qglTexCoord2f(0,1);
		qglVertex3f(0,0,0);
		qglTexCoord2f(1,1);
		qglVertex3f(glwidth/2,0,0);
		qglTexCoord2f(1,0);
		qglVertex3f(glwidth/2,glheight/2,0);
		qglTexCoord2f(0,0);
		qglVertex3f(0,glheight/2,0);
		qglEnd();
		qglEnable(GL_ALPHA_TEST);
	}
	else if(r_cloak_debug->value)
	{
		qglDisable(GL_BLEND);
		qglDisable(GL_ALPHA_TEST);
		qglColor4f(1,1,1,1);

		qglEnable(GL_TEXTURE_2D);
		qglBindTexture(GL_TEXTURE_2D, cloak_texture);

		qglBegin(GL_QUADS);
		qglTexCoord2f(0,1);
		qglVertex3f(0,0,0);
		qglTexCoord2f(1,1);
		qglVertex3f(glwidth/2,0,0);
		qglTexCoord2f(1,0);
		qglVertex3f(glwidth/2,glheight/2,0);
		qglTexCoord2f(0,0);
		qglVertex3f(0,glheight/2,0);
		qglEnd();
		qglEnable(GL_ALPHA_TEST);
	}
	else if(r_hudinworld_debug->value && r_hudinworld_texture)
	{
		qglDisable(GL_BLEND);
		qglDisable(GL_ALPHA_TEST);
		qglColor4f(1,1,1,1);

		qglEnable(GL_TEXTURE_2D);
		qglBindTexture(GL_TEXTURE_2D, r_hudinworld_texture);

		qglBegin(GL_QUADS);
		qglTexCoord2f(0,1);
		qglVertex3f(0,0,0);
		qglTexCoord2f(1,1);
		qglVertex3f(glwidth/2,0,0);
		qglTexCoord2f(1,0);
		qglVertex3f(glwidth/2,glheight/2,0);
		qglTexCoord2f(0,0);
		qglVertex3f(0,glheight/2,0);
		qglEnd();
		qglEnable(GL_ALPHA_TEST);
	}
	else if(r_hdr_debug->value)
	{
		qglDisable(GL_BLEND);
		qglDisable(GL_ALPHA_TEST);
		qglColor4f(1,1,1,1);

		qglEnable(GL_TEXTURE_2D);
		FBO_Container_t *pFBO = NULL;
		switch((int)r_hdr_debug->value)
		{
		case 1:
			pFBO = &s_DownSampleFBO[0];break;
		case 2:
			pFBO = &s_DownSampleFBO[1];break;
		case 3:
			pFBO = &s_BrightPassFBO;break;
		case 4:
			pFBO = &s_BlurPassFBO[0][0];break;
		case 5:
			pFBO = &s_BlurPassFBO[0][1];break;
		case 6:
			pFBO = &s_BlurPassFBO[1][0];break;
		case 7:
			pFBO = &s_BlurPassFBO[1][1];break;
		case 8:
			pFBO = &s_BlurPassFBO[2][0];break;
		case 9:
			pFBO = &s_BlurPassFBO[2][1];break;
		case 10:
			pFBO = &s_BrightAccumFBO;break;
		case 11:
			pFBO = &s_ToneMapFBO;break;
		default:
			break;
		}

		if(pFBO)
		{
			qglBindTexture(GL_TEXTURE_2D, pFBO->s_hBackBufferTex);
			qglBegin(GL_QUADS);
			qglTexCoord2f(0,1);
			qglVertex3f(0,0,0);
			qglTexCoord2f(1,1);
			qglVertex3f(glwidth/2, 0,0);
			qglTexCoord2f(1,0);
			qglVertex3f(glwidth/2, glheight/2,0);
			qglTexCoord2f(0,0);
			qglVertex3f(0, glheight/2,0);
			qglEnd();
		}
	}
	return gExportfuncs.HUD_Redraw(time, intermission);
}

int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio)
{
	DWORD addr;

	//Save StudioAPI Funcs
	gRefFuncs.studioapi_StudioDrawPoints = pstudio->StudioDrawPoints;
	gRefFuncs.studioapi_StudioSetupLighting = pstudio->StudioSetupLighting;
	gRefFuncs.studioapi_SetupRenderer = pstudio->SetupRenderer;
	gRefFuncs.studioapi_RestoreRenderer = pstudio->RestoreRenderer;

	//Vars in Engine Studio API
	addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)pstudio->GetCurrentEntity, 0x10, "\xA1", 1);
	Sig_AddrNotFound("currententity");
	currententity = *(cl_entity_t ***)(addr + 0x1);

	addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)pstudio->SetRenderModel, 0x10, "\xA3", 1);
	Sig_AddrNotFound("r_model");
	r_model = *(model_t ***)(addr + 1);

	addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)pstudio->StudioSetHeader, 0x10, "\xA3", 1);
	Sig_AddrNotFound("pstudiohdr");
	pstudiohdr = *(studiohdr_t ***)(addr + 1);

	addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)pstudio->SetForceFaceFlags, 0x10, "\xA3", 1);
	Sig_AddrNotFound("g_ForcedFaceFlags");
	g_ForcedFaceFlags = *(int **)(addr + 1);
	
	//call	CL_FxBlend
	addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)pstudio->StudioSetRenderamt, 0x50, "\xE8", 1);
	Sig_AddrNotFound("CL_FxBlend");
	gRefFuncs.CL_FxBlend = (int (*)(cl_entity_t *))GetCallAddress(addr);

	//fstp    r_blend
	addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)pstudio->StudioSetRenderamt, 0x50, "\xD9\x1D", sizeof("\xD9\x1D")-1);
	Sig_AddrNotFound("r_blend");
	r_blend = *(float **)(addr + 2);

	pbonetransform = (float (*)[MAXSTUDIOBONES][3][4])pstudio->StudioGetBoneTransform();
	plighttransform = (float (*)[MAXSTUDIOBONES][3][4])pstudio->StudioGetLightTransform();

	cl_viewent = gEngfuncs.GetViewModel();

	//Save Studio API
	memcpy(&IEngineStudio, pstudio, sizeof(IEngineStudio));
	gpStudioInterface = ppinterface;

	//InlineHook StudioAPI
	//InstallHook(studioapi_StudioDrawPoints);
	//InstallHook(studioapi_StudioSetupLighting);
	//InstallHook(studioapi_SetupRenderer);
	//InstallHook(studioapi_RestoreRenderer);

	R_InitDetailTextures();
	//Load global extra textures into array
	R_LoadExtraTextureFile(false);
	R_LoadStudioTextures(false);

	return gExportfuncs.HUD_GetStudioModelInterface(version, ppinterface, pstudio);
}

int HUD_UpdateClientData(client_data_t *pcldata, float flTime)
{
	scr_fov_value = pcldata->fov;
	return gExportfuncs.HUD_UpdateClientData(pcldata, flTime);
}

int HUD_AddEntity(int type, cl_entity_t *ent, const char *model)
{
	if(r_shadow->value && shadow.program && (type == 0 || type == 1))//NORMAL OR PLAYER
	{
		R_AddEntityShadow(ent, model);
	}
	if(r_3dsky_parm.enable)
	{
		if(ent->curstate.origin[0] + ent->curstate.maxs[0] > r_3dsky_parm.mins[0] && 
			ent->curstate.origin[1] + ent->curstate.maxs[1] > r_3dsky_parm.mins[1] &&
			ent->curstate.origin[2] + ent->curstate.maxs[2] > r_3dsky_parm.mins[2] && 
			ent->curstate.origin[0] + ent->curstate.mins[0] < r_3dsky_parm.maxs[0] && 
			ent->curstate.origin[1] + ent->curstate.mins[1] < r_3dsky_parm.maxs[1] && 
			ent->curstate.origin[2] + ent->curstate.mins[2] < r_3dsky_parm.maxs[2])
		{
			if(!r_3dsky->value)
				return 0;

			R_Add3DSkyEntity(ent);
		}
	}
	return gExportfuncs.HUD_AddEntity(type, ent, model);
}

void HUD_Frame(double time)
{
	for(shadowlight_t *sl = sdlights_active; sl; sl = sl->next)
	{
		sl->free = true;
	}
	return gExportfuncs.HUD_Frame(time);
}