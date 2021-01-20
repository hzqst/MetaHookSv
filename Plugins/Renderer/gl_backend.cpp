#include "gl_local.h"

byte *scrcapture_buffer = NULL;
int scrcapture_bufsize = 0;

void R_CaptureScreen(const char *szExt)
{
	int iFileIndex = 0;
	char *pLevelName;
	char szLevelName[64];
	char szFileName[260];

	if(s_BackBufferFBO.s_hBackBufferFBO)
	{
		int current_readframebuffer;
		qglGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &current_readframebuffer);

		if(current_readframebuffer != s_BackBufferFBO.s_hBackBufferFBO)
		{
			qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
			qglReadPixels(0, 0, glwidth, glheight, GL_RGB, GL_UNSIGNED_BYTE, scrcapture_buffer);
			qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, current_readframebuffer);
		}
		else
		{
			qglReadPixels(0, 0, glwidth, glheight, GL_RGB, GL_UNSIGNED_BYTE, scrcapture_buffer);
		}
	}
	else
	{
		qglReadPixels(0, 0, glwidth, glheight, GL_RGB, GL_UNSIGNED_BYTE, scrcapture_buffer);
	}

	const char *pLevel = gEngfuncs.pfnGetLevelName();
	if(!pLevel || !pLevel[0])
	{
		strcpy(szLevelName, "screenshot");
	}
	else
	{
		pLevelName = (char *)pLevel + strlen(pLevel) - 1;
		while( *pLevelName != '/' && *pLevelName != '\\')
			pLevelName --;
		strcpy(szLevelName, pLevelName+1);
		szLevelName[strlen(szLevelName)-4] = 0;
	}

	do
	{
		sprintf(szFileName, "%s%.4d.%s", szLevelName, iFileIndex, szExt);
		++iFileIndex;
	}while( true == g_pFileSystem->FileExists(szFileName) );

	if(TRUE == SaveImageGeneric(szFileName, glwidth, glheight, scrcapture_buffer))
	{
		gEngfuncs.Con_Printf("Screenshot %s saved.\n", szFileName);
	}
}

void CL_ScreenShot_f(void)
{
	if(scrcapture_bufsize < glwidth*glheight*3)
	{
		if(scrcapture_buffer)
			delete []scrcapture_buffer;
		scrcapture_bufsize = glwidth*glheight*3;
		scrcapture_buffer = new byte[scrcapture_bufsize];
	}

	char *szExt = "jpg";
	if(gEngfuncs.Cmd_Argc() > 1)
	{
		szExt = gEngfuncs.Cmd_Argv(1);
	}

	R_CaptureScreen(szExt);
}

byte *R_GetSCRCaptureBuffer(int *bufsize)
{
	if(bufsize)
		*bufsize = scrcapture_bufsize;
	return scrcapture_buffer;
}