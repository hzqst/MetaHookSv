#pragma once

#include <interface.h>
#include <string>

class IResourceReplacer : public IBaseInterface
{
public:
	virtual void FreeMapEntries() = 0;
	virtual void FreeGlobalEntries() = 0;
	virtual void Shutdown() = 0;
	virtual void LoadGlobalReplaceList(const char* szFileName) = 0;
	virtual void LoadMapReplaceList(const char* szFileName) = 0;
	virtual bool ReplaceFileName(const char *szSourceFileName, std::string &ReplacedFileName) = 0;
};

IResourceReplacer* ModelReplacer();
IResourceReplacer* SoundReplacer();