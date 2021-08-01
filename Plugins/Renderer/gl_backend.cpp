#include "gl_local.h"

void VID_CaptureScreen(const char *szExtension)
{
	int iFileIndex = 0;
	char szLevelName[64];
	char szFileName[1024];

	byte *pBuf = (byte *)malloc(glwidth * glheight * 3);

	GL_PushFrameBuffer();

	qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
	qglReadPixels(0, 0, glwidth, glheight, GL_RGB, GL_UNSIGNED_BYTE, pBuf);

	GL_PopFrameBuffer();

	if(r_worldmodel && r_worldmodel->name[0])
	{
		COM_FileBase(r_worldmodel->name, szLevelName);
	}
	else
	{		
		strcpy(szLevelName, "screenshot");
	}

	do
	{
		snprintf(szFileName, 1023, "%s%.4d.%s", szLevelName, iFileIndex, szExtension);
		szFileName[1023] = 0;

		++iFileIndex;
	}while( true == g_pFileSystem->FileExists(szFileName) );

	if(TRUE == SaveImageGeneric(szFileName, glwidth, glheight, pBuf))
	{
		gEngfuncs.Con_Printf("Screenshot %s saved.\n", szFileName);
	}

	free(pBuf);
}

void CL_ScreenShot_f(void)
{
	char *szExtension = "jpg";
	if(gEngfuncs.Cmd_Argc() > 1)
	{
		szExtension = gEngfuncs.Cmd_Argv(1);
	}

	VID_CaptureScreen(szExtension);
}