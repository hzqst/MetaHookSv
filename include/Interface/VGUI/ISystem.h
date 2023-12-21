#ifndef ISYSTEM_H
#define ISYSTEM_H

#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include <vgui/VGUI.h>
#include <vgui/KeyCode.h>

#ifdef PlaySound
#undef PlaySound
#endif

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

class KeyValues;

namespace vgui
{

class ISystem : public IBaseInterface
{
public:
	virtual void Shutdown(void) = 0;
	virtual void RunFrame(void) = 0;
	virtual void ShellExecute(const char *command, const char *file) = 0;
	virtual double GetFrameTime(void) = 0;
	virtual double GetCurrentTime(void) = 0;
	virtual long GetTimeMillis(void) = 0;
	virtual int GetClipboardTextCount(void) = 0;
	virtual void SetClipboardText(const char *text, int textLen) = 0;
	virtual void SetClipboardText(const wchar_t *text, int textLen) = 0;
	virtual int GetClipboardText(int offset, char *buf, int bufLen) = 0;
	virtual int GetClipboardText(int offset, wchar_t *buf, int bufLen) = 0;
	virtual bool SetRegistryString(const char *key, const char *value) = 0;
	virtual bool GetRegistryString(const char *key, char *value, int valueLen) = 0;
	virtual bool SetRegistryInteger(const char *key, int value) = 0;
	virtual bool GetRegistryInteger(const char *key, int &value) = 0;
	virtual KeyValues *GetUserConfigFileData(const char *dialogName, int dialogID) = 0;
	virtual void SetUserConfigFile(const char *fileName, const char *pathName) = 0;
	virtual void SaveUserConfigFile(void) = 0;
	virtual bool SetWatchForComputerUse(bool state) = 0;
	virtual double GetTimeSinceLastUse(void) = 0;
	virtual int GetAvailableDrives(char *buf, int bufLen) = 0;
	virtual bool CommandLineParamExists(const char *paramName) = 0;
	virtual const char *GetFullCommandLine(void) = 0;
	virtual bool GetCurrentTimeAndDate(int *year, int *month, int *dayOfWeek, int *day, int *hour, int *minute, int *second) = 0;
	virtual double GetFreeDiskSpace(const char *path) = 0;
	virtual bool CreateShortcut(const char *linkFileName, const char *targetPath, const char *arguments, const char *workingDirectory, const char *iconFile) = 0;
	virtual bool GetShortcutTarget(const char *linkFileName, char *targetPath, char *arguments, int destBufferSizes) = 0;
	virtual bool ModifyShortcutTarget(const char *linkFileName, const char *targetPath, const char *arguments, const char *workingDirectory) = 0;
	virtual bool GetCommandLineParamValue(const char *paramName, char *value, int valueBufferSize) = 0;
	virtual bool DeleteRegistryKey(const char *keyName) = 0;
	virtual const char *GetDesktopFolderPath(void) = 0;
};
}

#define VGUI_SYSTEM_INTERFACE_VERSION "VGUI_System009"

#endif