#ifndef IPLUGINS_V1_H
#define IPLUGINS_V1_H

#ifdef _WIN32
#pragma once
#endif

class IPluginsV1 : public IBaseInterface
{
public:
	virtual void Init(struct cl_exportfuncs_s *pExportfuncs);
	virtual void Shutdown(int restart);
};

#define METAHOOK_PLUGIN_API_VERSION_V1 "METAHOOK_PLUGIN_API_VERSION001"

#endif