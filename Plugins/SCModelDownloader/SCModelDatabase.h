#pragma once

#include <interface.h>

class ISCModelDatabase : public IBaseInterface
{
public:
	virtual void Init() = 0;
	virtual void Shutdown() = 0;
	virtual void RunFrame() = 0;
	virtual void OnMissingModel(const char* modelname) = 0;
};

ISCModelDatabase* SCModelDatabase();

#define SCMODEL_DATABASE_INTERFACE_VERSION "SCModelDatabase_001"