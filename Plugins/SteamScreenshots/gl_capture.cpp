#include <metahook.h>
#include <glew.h>
#include "gl_capture.h"

GLuint g_CapturePBO = 0;
GLsync g_CaptureSyncObject = 0;
int g_CaptureImageWidth = 0;
int g_CaptureImageHeight = 0;
void* g_CaptureImageBuffer = NULL;
void (*g_pfnBeginCapture)(fnGLQueryCaptureCallback callback) = NULL;

void GL_InitCaptureImageBuffer(int width, int height)
{
	g_CaptureImageBuffer = (byte*)malloc(width * height * 3);
}

void GL_ShutdownCaptureImageBuffer()
{
	if (g_CaptureImageBuffer)
	{
		free(g_CaptureImageBuffer);
		g_CaptureImageBuffer = NULL;
	}
}

void GL_InitCapturePBO(int width, int height)
{
	int originalPBO = 0;
	glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &originalPBO);

	glGenBuffers(1, &g_CapturePBO);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, g_CapturePBO);
	glBufferData(GL_PIXEL_PACK_BUFFER, width * height * 3, 0, GL_STREAM_READ);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, originalPBO);
}

void GL_ShutdownCapturePBO()
{
	if (g_CapturePBO)
	{
		glDeleteBuffers(1, &g_CapturePBO);
		g_CapturePBO = 0;
	}
}

void GL_ShutdownCaptureSyncObject()
{
	if (g_CaptureSyncObject)
	{
		glDeleteSync(g_CaptureSyncObject);
		g_CaptureSyncObject = 0;
	}
}

static void GL_ReadCapturePixels(GLuint pbo, void* pixels)
{
	int originalFBO = 0;
	int originalPBO = 0;
	int originalAlignment = 0;
	int originalRowLength = 0;
	int originalSkipRows = 0;
	int originalSkipPixels = 0;
	const bool hasPBO = (GLEW_VERSION_2_1 || GLEW_ARB_pixel_buffer_object) && glBindBuffer;

	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &originalFBO);
	glGetIntegerv(GL_PACK_ALIGNMENT, &originalAlignment);
	glGetIntegerv(GL_PACK_ROW_LENGTH, &originalRowLength);
	glGetIntegerv(GL_PACK_SKIP_ROWS, &originalSkipRows);
	glGetIntegerv(GL_PACK_SKIP_PIXELS, &originalSkipPixels);
	if (hasPBO)
	{
		glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &originalPBO);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
	}
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	// Both the allocation and the image flip use tightly packed RGB rows.
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	glPixelStorei(GL_PACK_SKIP_ROWS, 0);
	glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
	glReadPixels(0, 0, g_CaptureImageWidth, g_CaptureImageHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	glPixelStorei(GL_PACK_ALIGNMENT, originalAlignment);
	glPixelStorei(GL_PACK_ROW_LENGTH, originalRowLength);
	glPixelStorei(GL_PACK_SKIP_ROWS, originalSkipRows);
	glPixelStorei(GL_PACK_SKIP_PIXELS, originalSkipPixels);
	if (hasPBO)
		glBindBuffer(GL_PIXEL_PACK_BUFFER, originalPBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, originalFBO);
}

void GL_BeginSyncCapture(fnGLQueryCaptureCallback callback)
{
	int glwidth, glheight;
	g_pMetaHookAPI->GetVideoMode(&glwidth, &glheight, NULL, NULL);

	if (glwidth != g_CaptureImageWidth || glheight != g_CaptureImageHeight)
	{
		GL_ShutdownCaptureImageBuffer();
		GL_InitCaptureImageBuffer(glwidth, glheight);

		g_CaptureImageWidth = glwidth;
		g_CaptureImageHeight = glheight;
	}
	
	GL_ReadCapturePixels(0, g_CaptureImageBuffer);

	GLubyte* pBuf = (GLubyte*)g_CaptureImageBuffer;

	//Flip the image up-side down
	for (int y = 0; y < g_CaptureImageHeight / 2; ++y) {
		for (int x = 0; x < g_CaptureImageWidth; ++x) {
			byte temp[3];
			memcpy(temp, &pBuf[(g_CaptureImageHeight - y - 1) * g_CaptureImageWidth * 3 + x * 3], 3);
			memcpy(&pBuf[(g_CaptureImageHeight - y - 1) * g_CaptureImageWidth * 3 + x * 3], &pBuf[y * g_CaptureImageWidth * 3 + x * 3], 3);
			memcpy(&pBuf[y * g_CaptureImageWidth * 3 + x * 3], temp, 3);
		}
	}

	callback(g_CaptureImageBuffer, g_CaptureImageWidth * g_CaptureImageHeight * 3, g_CaptureImageWidth, g_CaptureImageHeight);
}

void GL_BeginAsyncCapture(fnGLQueryCaptureCallback callback)
{
	if (g_CaptureSyncObject)
		return;

	int glwidth, glheight;
	g_pMetaHookAPI->GetVideoMode(&glwidth, &glheight, NULL, NULL);

	if (glwidth != g_CaptureImageWidth || glheight != g_CaptureImageHeight)
	{
		GL_ShutdownCaptureImageBuffer();
		GL_ShutdownCapturePBO();
		GL_InitCapturePBO(glwidth, glheight);
		GL_InitCaptureImageBuffer(glwidth, glheight);

		g_CaptureImageWidth = glwidth;
		g_CaptureImageHeight = glheight;
	}

	GL_ReadCapturePixels(g_CapturePBO, nullptr);

	g_CaptureSyncObject = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void GL_BeginCapture(fnGLQueryCaptureCallback callback)
{
	return g_pfnBeginCapture(callback);
}

void GL_QueryAsyncCapture(fnGLQueryCaptureCallback callback)
{
	if (!g_CaptureSyncObject)
		return;

	GLenum wait = glClientWaitSync(g_CaptureSyncObject, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
	if (wait == GL_WAIT_FAILED)
	{
		GL_ShutdownCaptureSyncObject();
		gEngfuncs.Con_Printf("[SteamScreenshots] Cannot capture screenshot: GPU fence wait failed.\n");
		return;
	}

	if (wait == GL_ALREADY_SIGNALED || wait == GL_CONDITION_SATISFIED)
	{
		int originalPBO = 0;
		glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &originalPBO);

		glBindBuffer(GL_PIXEL_PACK_BUFFER, g_CapturePBO);

		GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
		if (ptr)
		{
			memcpy(g_CaptureImageBuffer, ptr, g_CaptureImageWidth * g_CaptureImageHeight * 3);

			GLubyte* pBuf = (GLubyte*)g_CaptureImageBuffer;

			//Flip the image up-side down
			for (int y = 0; y < g_CaptureImageHeight / 2; ++y) {
				for (int x = 0; x < g_CaptureImageWidth; ++x) {
					byte temp[3];
					memcpy(temp, &pBuf[(g_CaptureImageHeight - y - 1) * g_CaptureImageWidth * 3 + x * 3], 3);
					memcpy(&pBuf[(g_CaptureImageHeight - y - 1) * g_CaptureImageWidth * 3 + x * 3], &pBuf[y * g_CaptureImageWidth * 3 + x * 3], 3);
					memcpy(&pBuf[y * g_CaptureImageWidth * 3 + x * 3], temp, 3);
				}
			}

			callback(g_CaptureImageBuffer, g_CaptureImageWidth * g_CaptureImageHeight * 3, g_CaptureImageWidth, g_CaptureImageHeight);

			glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		}

		glBindBuffer(GL_PIXEL_PACK_BUFFER, originalPBO);

		glDeleteSync(g_CaptureSyncObject);
		g_CaptureSyncObject = 0;
	}
}

void GL_ShutdownCapture()
{
	GL_ShutdownCaptureImageBuffer();
	GL_ShutdownCapturePBO();
	GL_ShutdownCaptureSyncObject();
}

bool GL_InitCapture()
{
	// GL_READ_FRAMEBUFFER requires core/ARB framebuffer support, not just EXT_framebuffer_object.
	if (!(GLEW_VERSION_3_0 || GLEW_ARB_framebuffer_object) || !glBindFramebuffer)
	{
		gEngfuncs.Con_Printf("[SteamScreenshots] Framebuffer capture unavailable; keeping the engine snapshot command.\n");
		return false;
	}

	g_pMetaHookAPI->GetVideoMode(&g_CaptureImageWidth, &g_CaptureImageHeight, NULL, NULL);

	if (GLEW_VERSION_3_2 && glFenceSync && glClientWaitSync)
	{
		GL_InitCapturePBO(g_CaptureImageWidth, g_CaptureImageHeight);
		g_pfnBeginCapture = GL_BeginAsyncCapture;
	}
	else
	{
		g_pfnBeginCapture = GL_BeginSyncCapture;
	}

	GL_InitCaptureImageBuffer(g_CaptureImageWidth, g_CaptureImageHeight);
	return true;
}
