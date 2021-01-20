#ifndef IPLUGINS_H
#define IPLUGINS_H

#ifdef _WIN32
#pragma once
#endif

class IPlugins : public IBaseInterface
{
public:
	virtual void Init(metahook_api_t *pAPI, mh_interface_t *pInterface, mh_enginesave_t *pSave);
	virtual void Shutdown(void);
	virtual void LoadEngine(void);
	virtual void LoadClient(cl_exportfuncs_t *pExportFunc);
	virtual void ExitGame(int iResult);
};

#define METAHOOK_PLUGIN_API_VERSION "METAHOOK_PLUGIN_API_VERSION002"

#endif