#pragma once

typedef void (*fnGLQueryCaptureCallback)(void* pBuf, size_t cbBufSize, int width, int height);

void GL_InitCapture();
void GL_ShutdownCapture();
void GL_BeginCapture(fnGLQueryCaptureCallback callback);
void GL_QueryAsyncCapture(fnGLQueryCaptureCallback callback);