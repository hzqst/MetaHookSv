#pragma once

#include <IUtilHTTPClient.h>

void UtilHTTPClient_InitSteamAPI(); 
void UtilHTTPClient_InitLibCurl();
void UtilHTTPClient_Init();
void UtilHTTPClient_RunFrame();
void UtilHTTPClient_Shutdown();

IUtilHTTPClient* UtilHTTPClient();