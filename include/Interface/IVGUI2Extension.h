#pragma once

#include <interface.h>
#include <stdint.h>
#include <stdarg.h>

#include <IGameUI.h>
#include <IClientVGUI.h>
#include <IFileSystem.h>
#include <VGUI\VGUI.h>

#define DEFINE_VGUI2EXTENSION_CALLBACK_SIMPLE(name, ...)  virtual void name(__VA_ARGS__) = 0;
#define DEFINE_VGUI2EXTENSION_CALLBACK(name, ...)  virtual void name(__VA_ARGS__, VGUI2Extension_CallbackContext* CallbackContext) = 0;
#define DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(name)  virtual void name(VGUI2Extension_CallbackContext* CallbackContext) = 0;

enum class VGUI2Extension_Result{
    UNSET = 0,
    IGNORED,		            // plugin didn't take any action
    HANDLED,		            // plugin did something, but real function should still be called
    OVERRIDE,		           // call real function, but use my return value
    SUPERCEDE,		            // skip real function; use my return value
    SUPERCEDE_SKIP_PLUGINS,		// skip real function and any other plugin calls; use my return value
};

class VGUI2Extension_CallbackContext
{
public:
    VGUI2Extension_CallbackContext()
    {
        Result = VGUI2Extension_Result::UNSET;
        pPluginReturnValue = nullptr;
        pRealReturnValue = nullptr;
        IsPost = false;
    }

    VGUI2Extension_Result Result;
    void* pPluginReturnValue;
    void* pRealReturnValue;
    bool IsPost;
};

class IVGUI2Extension_String : public IBaseInterface
{
public:
   virtual const char* c_str() const = 0;
   virtual const char* data() const = 0;
   virtual size_t length() const = 0;
   virtual size_t capacity() const = 0;
   virtual void resize(size_t n) = 0;
   virtual void assign(const char*s) = 0;
   virtual void assign2(const char *s, size_t n) = 0;
   virtual void clear() = 0;
};

class IVGUI2Extension_BaseCallbacks : public IBaseInterface
{
public:
    virtual int GetAltitude() const = 0;
};

class IVGUI2Extension_BaseUICallbacks : public IVGUI2Extension_BaseCallbacks
{
public:
    DEFINE_VGUI2EXTENSION_CALLBACK_SIMPLE(Initialize, CreateInterfaceFn* factories, int count);
    DEFINE_VGUI2EXTENSION_CALLBACK_SIMPLE(Start, struct cl_enginefuncs_s* engineFuncs, int interfaceVersion);
    DEFINE_VGUI2EXTENSION_CALLBACK_SIMPLE(Shutdown);
    DEFINE_VGUI2EXTENSION_CALLBACK(Key_Event, int& down, int& keynum, const char*& pszCurrentBinding);
    DEFINE_VGUI2EXTENSION_CALLBACK(CallEngineSurfaceAppProc, void* &pevent, void* &userData);
    DEFINE_VGUI2EXTENSION_CALLBACK(CallEngineSurfaceWndProc, void* &hwnd, unsigned int &msg, unsigned int &wparam, long &lparam);
    DEFINE_VGUI2EXTENSION_CALLBACK(Paint, int& x, int& y, int& right, int& bottom);
    DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(HideGameUI);
    DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(ActivateGameUI);
    DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(HideConsole);
    DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(ShowConsole);
};

class IGameUIOptionsDialogCtorCallbackContext : public IBaseInterface
{
public:
    virtual void* GetDialog() const = 0;
    virtual void* GetPropertySheet() const = 0;
    virtual void AddPage(void* panel, const char* title) = 0;
};

class IVGUI2Extension_GameUIOptionDialogCallbacks : public IVGUI2Extension_BaseCallbacks
{
public:
    DEFINE_VGUI2EXTENSION_CALLBACK_SIMPLE(COptionsDialog_ctor, IGameUIOptionsDialogCtorCallbackContext* CallbackContext);
    DEFINE_VGUI2EXTENSION_CALLBACK(COptionsSubVideo_ApplyVidSettings, void*& pPanel, bool& bForceRestart);
};

class IGameUITaskBarCtorCallbackContext : public IBaseInterface
{
public:
    virtual void* GetTaskBar() const = 0;
    virtual void* GetParentPanel() const = 0;
    virtual const char* GetParentName() const = 0;
};

class IVGUI2Extension_GameUITaskBarCallbacks : public IVGUI2Extension_BaseCallbacks
{
public:
    DEFINE_VGUI2EXTENSION_CALLBACK_SIMPLE(CTaskBar_ctor, IGameUITaskBarCtorCallbackContext* CallbackContext);
    DEFINE_VGUI2EXTENSION_CALLBACK(CTaskBar_OnCommand, void*& pPanel, const char*& command);
};

class IVGUI2Extension_KeyValuesCallbacks : public IVGUI2Extension_BaseCallbacks
{
public:
    DEFINE_VGUI2EXTENSION_CALLBACK(KeyValues_LoadFromFile, void* &pthis, IFileSystem* &pFileSystem, const char* &resourceName, const char* &pathId, const char *sourceModule);
};

class IVGUI2Extension_GameUICallbacks : public IVGUI2Extension_BaseCallbacks
{
public:
    DEFINE_VGUI2EXTENSION_CALLBACK_SIMPLE(Initialize, CreateInterfaceFn* factories, int count);
    DEFINE_VGUI2EXTENSION_CALLBACK_SIMPLE(Start, struct cl_enginefuncs_s* engineFuncs, int interfaceVersion, void* system);
    DEFINE_VGUI2EXTENSION_CALLBACK_SIMPLE(Shutdown);
    DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(ActivateGameUI);
    DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(ActivateDemoUI);
    DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(HasExclusiveInput);
    DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(RunFrame);
    DEFINE_VGUI2EXTENSION_CALLBACK(ConnectToServer, const char* &game, int &IP, int &port);
    DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(DisconnectFromServer);
    DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(HideGameUI);
    DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(IsGameUIActive);
    DEFINE_VGUI2EXTENSION_CALLBACK(LoadingStarted, const char* &resourceType, const char* &resourceName);
    DEFINE_VGUI2EXTENSION_CALLBACK(LoadingFinished, const char* &resourceType, const char*& resourceName);
    DEFINE_VGUI2EXTENSION_CALLBACK(StartProgressBar, const char* &progressType, int &progressSteps);
    DEFINE_VGUI2EXTENSION_CALLBACK(ContinueProgressBar, int &progressPoint, float &progressFraction);
    DEFINE_VGUI2EXTENSION_CALLBACK(StopProgressBar, bool &bError, const char* &failureReason, const char* &extendedReason);
    DEFINE_VGUI2EXTENSION_CALLBACK(SetProgressBarStatusText, const char* &statusText);
    DEFINE_VGUI2EXTENSION_CALLBACK(SetSecondaryProgressBar, float &progress);
    DEFINE_VGUI2EXTENSION_CALLBACK(SetSecondaryProgressBarText, const char* &statusText);

    //For inter-module controls
    virtual const char *GetControlModuleName() const = 0;
};

class IVGUI2Extension_ClientVGUICallbacks : public IVGUI2Extension_BaseCallbacks
{
public:
    DEFINE_VGUI2EXTENSION_CALLBACK_SIMPLE(Initialize, CreateInterfaceFn* factories, int count);
    DEFINE_VGUI2EXTENSION_CALLBACK_SIMPLE(Shutdown);
    DEFINE_VGUI2EXTENSION_CALLBACK_SIMPLE(Start);
    DEFINE_VGUI2EXTENSION_CALLBACK_SIMPLE(SetParent, vgui::VPANEL parent);
    DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(UseVGUI1);
    DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(HideScoreBoard);
    DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(HideAllVGUIMenu);
    DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(ActivateClientUI);
    DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(HideClientUI);
};

class IVGUI2Extension_GameConsoleCallbacks : public IVGUI2Extension_BaseCallbacks
{
public:
    DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(Activate);
    DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(Initialize);
    DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(Hide);
    DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(Clear);
    DEFINE_VGUI2EXTENSION_CALLBACK_NOARG(IsConsoleVisible);
    DEFINE_VGUI2EXTENSION_CALLBACK(Printf, IVGUI2Extension_String *str);
    DEFINE_VGUI2EXTENSION_CALLBACK(DPrintf, IVGUI2Extension_String* str);
    DEFINE_VGUI2EXTENSION_CALLBACK(SetParent, vgui::VPANEL parent);
};

class IVGUI2Extension : public IBaseInterface
{
public:
    virtual void RegisterBaseUICallbacks(IVGUI2Extension_BaseUICallbacks* pCallbacks) = 0;
    virtual void RegisterGameUICallbacks(IVGUI2Extension_GameUICallbacks* pCallbacks) = 0;
    virtual void RegisterGameUIOptionDialogCallbacks(IVGUI2Extension_GameUIOptionDialogCallbacks* pCallbacks) = 0;
    virtual void RegisterGameUITaskBarCallbacks(IVGUI2Extension_GameUITaskBarCallbacks* pCallbacks) = 0;
    virtual void RegisterGameConsoleCallbacks(IVGUI2Extension_GameConsoleCallbacks* pCallbacks) = 0;
    virtual void RegisterClientVGUICallbacks(IVGUI2Extension_ClientVGUICallbacks* pCallbacks) = 0;
    virtual void RegisterKeyValuesCallbacks(IVGUI2Extension_KeyValuesCallbacks* pCallbacks) = 0;

    virtual void UnregisterBaseUICallbacks(IVGUI2Extension_BaseUICallbacks* pCallbacks) = 0;
    virtual void UnregisterGameUICallbacks(IVGUI2Extension_GameUICallbacks* pCallbacks) = 0;
    virtual void UnregisterGameUIOptionDialogCallbacks(IVGUI2Extension_GameUIOptionDialogCallbacks* pCallbacks) = 0;
    virtual void UnregisterGameUITaskBarCallbacks(IVGUI2Extension_GameUITaskBarCallbacks* pCallbacks) = 0;
    virtual void UnregisterGameConsoleCallbacks(IVGUI2Extension_GameConsoleCallbacks* pCallbacks) = 0;
    virtual void UnregisterClientVGUICallbacks(IVGUI2Extension_ClientVGUICallbacks* pCallbacks) = 0;
    virtual void UnregisterKeyValuesCallbacks(IVGUI2Extension_KeyValuesCallbacks* pCallbacks) = 0;

    virtual const char* GetBaseDirectory() const = 0;
    virtual const char* GetCurrentLanguage() const = 0;
};

IVGUI2Extension* VGUI2Extension();

#define VGUI2_EXTENSION_INTERFACE_VERSION "VGUI2_Extension_API_007"