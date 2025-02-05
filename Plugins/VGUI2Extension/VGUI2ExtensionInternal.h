#pragma once

#include <metahook.h>
#include <IVGUI2Extension.h>
#include <string>

#define DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_SIMPLE(name, ...) virtual void name(__VA_ARGS__) = 0;
#define DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK(name, ...)  virtual void name(__VA_ARGS__, VGUI2Extension_CallbackContext* CallbackContext) = 0;
#define DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(name)  virtual void name(VGUI2Extension_CallbackContext* CallbackContext) = 0;

class CVGUI2Extension_String : public IVGUI2Extension_String
{
private:
    std::string m_str;
public:
    const char* c_str() const override
    {
        return m_str.c_str();
    }
    const char* data() const override
    {
        return m_str.data();
    }
    size_t length() const override
    {
        return m_str.length();
    }
    size_t capacity() const override
    {
        return m_str.capacity();
    }
    void resize(size_t n) override
    {
        m_str.resize(n);
    }
    void assign(const char* s) override
    {
        m_str.assign(s);
    }
    void assign2(const char* s, size_t n) override
    {
        m_str.assign(s, n);
    }
    void clear() override
    {
        m_str.clear();
    }
};

class IVGUI2ExtensionInternal : public IVGUI2Extension
{
public:
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_SIMPLE(BaseUI_Initialize, CreateInterfaceFn* factories, int count);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_SIMPLE(BaseUI_Start, struct cl_enginefuncs_s* engineFuncs, int interfaceVersion);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_SIMPLE(BaseUI_Shutdown);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK(BaseUI_Key_Event, int& down, int& keynum, const char*& pszCurrentBinding);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK(BaseUI_CallEngineSurfaceAppProc, void*& pevent, void*& userData);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK(BaseUI_CallEngineSurfaceWndProc, void*& hwnd, unsigned int &msg, unsigned int& wparam, long& lparam);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK(BaseUI_Paint, int& x, int& y, int& right, int& bottom);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(BaseUI_HideGameUI);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(BaseUI_ActivateGameUI);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(BaseUI_HideConsole);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(BaseUI_ShowConsole);

    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_SIMPLE(GameUI_Initialize, CreateInterfaceFn* factories, int count);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_SIMPLE(GameUI_PreStart, struct cl_enginefuncs_s* engineFuncs, int interfaceVersion, void* system);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_SIMPLE(GameUI_Start, struct cl_enginefuncs_s* engineFuncs, int interfaceVersion, void* system);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_SIMPLE(GameUI_Shutdown);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_SIMPLE(GameUI_PostShutdown);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(GameUI_ActivateGameUI);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(GameUI_ActivateDemoUI);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(GameUI_HasExclusiveInput);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(GameUI_RunFrame);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK(GameUI_ConnectToServer, const char*& game, int& IP, int& port);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(GameUI_DisconnectFromServer);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(GameUI_HideGameUI);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(GameUI_IsGameUIActive);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK(GameUI_LoadingStarted, const char*& resourceType, const char*& resourceName);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK(GameUI_LoadingFinished, const char*& resourceType, const char*& resourceName);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK(GameUI_StartProgressBar, const char*& progressType, int& progressSteps);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK(GameUI_ContinueProgressBar, int& progressPoint, float& progressFraction);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK(GameUI_StopProgressBar, bool& bError, const char*& failureReason, const char*& extendedReason);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK(GameUI_SetProgressBarStatusText, const char*& statusText);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK(GameUI_SetSecondaryProgressBar, float& progress);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK(GameUI_SetSecondaryProgressBarText, const char*& statusText);

    virtual const char *GameUI_GetControlModuleName(int i) const = 0;
    virtual int GameUI_GetCallbackCount() const = 0;

    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_SIMPLE(GameUI_COptionsDialog_ctor, IGameUIOptionsDialogCtorCallbackContext* CallbackContext);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK(GameUI_COptionsSubVideo_ApplyVidSettings, void*& pPanel, bool& bForceRestart);

    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_SIMPLE(GameUI_CTaskBar_ctor, IGameUITaskBarCtorCallbackContext* CallbackContext);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK(GameUI_CTaskBar_OnCommand, void*& pPanel, const char*& command);

    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK(KeyValues_LoadFromFile, void*& pthis, IFileSystem*& pFileSystem, const char*& resourceName, const char*& pathId, const char* sourceModule);

    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(GameConsole_Activate);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(GameConsole_Initialize);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(GameConsole_Hide);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(GameConsole_Clear);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(GameConsole_IsConsoleVisible);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK(GameConsole_Printf, IVGUI2Extension_String* str);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK(GameConsole_DPrintf, IVGUI2Extension_String* str);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK(GameConsole_SetParent, vgui::VPANEL parent);

    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_SIMPLE(ClientVGUI_Initialize, CreateInterfaceFn* factories, int count);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_SIMPLE(ClientVGUI_Shutdown);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_SIMPLE(ClientVGUI_Start);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_SIMPLE(ClientVGUI_SetParent, vgui::VPANEL parent);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(ClientVGUI_UseVGUI1);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(ClientVGUI_HideScoreBoard);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(ClientVGUI_HideAllVGUIMenu);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(ClientVGUI_ActivateClientUI);
    DEFINE_VGUI2EXTENSION_INTERNAL_CALLBACK_NOARG(ClientVGUI_HideClientUI);
};

IVGUI2ExtensionInternal* VGUI2ExtensionInternal();