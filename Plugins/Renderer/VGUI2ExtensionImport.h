#pragma once

#include <IVGUI2Extension.h>
#include <IDpiManager.h>

void VGUI2Extension_Init();
void VGUI2Extension_Shutdown();

void BaseUI_InstallHooks(void);
void BaseUI_UninstallHooks(void);

void GameUI_InstallHooks(void);
void GameUI_UninstallHooks(void);