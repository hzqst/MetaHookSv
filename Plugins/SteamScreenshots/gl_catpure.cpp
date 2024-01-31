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
	glGenBuffers(1, &g_CapturePBO);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, g_CapturePBO);
	glBufferData(GL_PIXEL_PACK_BUFFER, width * height * 3, 0, GL_STREAM_READ);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
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
	
	int originalFBO = 0;
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &originalFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	glReadPixels(0, 0, g_CaptureImageWidth, g_CaptureImageHeight, GL_RGB, GL_UNSIGNED_BYTE, g_CaptureImageBuffer);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, originalFBO);

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

	int originalFBO = 0;
	int originalPBO = 0;
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &originalFBO);
	glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &originalPBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, g_CapturePBO);

	glReadPixels(0, 0, g_CaptureImageWidth, g_CaptureImageHeight, GL_RGB, GL_UNSIGNED_BYTE, 0);

	g_CaptureSyncObject = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, originalPBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, originalFBO);
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

void GL_InitCapture()
{
	g_pMetaHookAPI->GetVideoMode(&g_CaptureImageWidth, &g_CaptureImageHeight, NULL, NULL);

	if (GLEW_VERSION_3_2)
	{
		GL_InitCapturePBO(g_CaptureImageWidth, g_CaptureImageHeight);
		g_pfnBeginCapture = GL_BeginAsyncCapture;
	}
	else
	{
		g_pfnBeginCapture = GL_BeginSyncCapture;
	}

	GL_InitCaptureImageBuffer(g_CaptureImageWidth, g_CaptureImageHeight);
}