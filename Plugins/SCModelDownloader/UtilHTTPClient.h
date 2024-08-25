#pragma once

#include <IUtilHTTPClient.h>

void UtilHTTPClient_Init();
void UtilHTTPClient_RunFrame();
void UtilHTTPClient_Shutdown();

IUtilHTTPClient* UtilHTTPClient();