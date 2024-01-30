#ifndef VGUI_INTERNAL_H
#define VGUI_INTERNAL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include "interface.h"
#include "tier3/tier3.h"
#include <ICommandLine.h>

namespace vgui
{

bool VGui_InternalLoadInterfaces(CreateInterfaceFn *factoryList, int numFactories);
bool VGui_LoadEngineInterfaces(CreateInterfaceFn vguiFactory, CreateInterfaceFn engineFactory);

}

extern class IEngineVGui *g_pEngineVGui;

extern struct cl_enginefuncs_s *engine;

#endif