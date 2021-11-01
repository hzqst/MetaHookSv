#ifndef IPLUGINS_H
#define IPLUGINS_H

#ifdef _WIN32
#pragma once
#endif

#include <builddefs.h>

class IPluginsV1 : public IBaseInterface
{
public:
	virtual void Init(struct cl_exportfuncs_s *pExportfuncs);
	virtual void Shutdown(int restart);
};

#define METAHOOK_PLUGIN_API_VERSION_V1 "METAHOOK_PLUGIN_API_VERSION001"

class IPluginsV2 : public IBaseInterface
{
public:
	virtual void Init(metahook_api_t *pAPI, mh_interface_t *pInterface, mh_enginesave_t *pSave);
	virtual void Shutdown(void);
	virtual void LoadEngine(void);
	virtual void LoadClient(cl_exportfuncs_t *pExportFunc);
	virtual void ExitGame(int iResult);
};

using IPlugins = IPluginsV2;

#define METAHOOK_PLUGIN_API_VERSION_V2 "METAHOOK_PLUGIN_API_VERSION002"
#define METAHOOK_PLUGIN_API_VERSION METAHOOK_PLUGIN_API_VERSION_V2

class IPluginsV3 : public IBaseInterface
{
public:
	virtual void Init(metahook_api_t *pAPI, mh_interface_t *pInterface, mh_enginesave_t *pSave);
	virtual void Shutdown(void);
	virtual void LoadEngine(cl_enginefunc_t *pEngineFuncs);
	virtual void LoadClient(cl_exportfuncs_t *pExportFuncs);
	virtual void ExitGame(int iResult);
};

#define METAHOOK_PLUGIN_API_VERSION_V3 "METAHOOK_PLUGIN_API_VERSION003"

class IPluginsV4 : public IBaseInterface
{
public:
	virtual void Init(metahook_api_t *pAPI, mh_interface_t *pInterface, mh_enginesave_t *pSave);
	virtual void Shutdown(void);
	virtual void LoadEngine(cl_enginefunc_t *pEngineFuncs);
	virtual void LoadClient(cl_exportfuncs_t *pExportFuncs);
	virtual void ExitGame(int iResult);
	virtual const char *GetVersion(void);
};

#define METAHOOK_PLUGIN_API_VERSION_V4 "METAHOOK_PLUGIN_API_VERSION004"

#endif