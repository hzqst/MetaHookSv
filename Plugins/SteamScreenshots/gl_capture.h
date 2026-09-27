#pragma once

typedef void (*fnGLQueryCaptureCallback)(void* pBuf, size_t cbBufSize, int width, int height);

bool GL_InitCapture();
void GL_ShutdownCapture();
void GL_RequestCapture();
void GL_DiscardPendingCapture();
bool GL_CapturePendingBeforeSwap(fnGLQueryCaptureCallback callback);
bool GL_BeginCapture(fnGLQueryCaptureCallback callback);
void GL_QueryAsyncCapture(fnGLQueryCaptureCallback callback);
