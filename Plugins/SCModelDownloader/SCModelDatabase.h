#pragma once

#include <interface.h>

class ISCModelQuery : public IBaseInterface
{
public:
	virtual const char* GetName() const = 0;
	virtual const char* GetIdentifier() const = 0;
	virtual const char* GetUrl() const = 0;
	virtual bool IsFailed() const = 0;
	virtual bool IsFinished() const = 0;

	virtual void OnFinish() = 0;
	virtual void OnFailure() = 0;
	virtual bool OnProcessPayload(const char* data, size_t size) = 0;

	virtual void RunFrame(float flCurrentAbsTime) = 0;
	virtual void StartQuery() = 0;
};

class IEnumSCModelQueryHandler : public IBaseInterface
{
public:
	virtual void OnBeforeEnum() = 0;
	virtual void OnEnum(ISCModelQuery* pQuery) = 0;
	virtual void OnEndEnum() = 0;
};

class ISCModelDatabase : public IBaseInterface
{
public:
	virtual void Init() = 0;
	virtual void Shutdown() = 0;
	virtual void RunFrame() = 0;
	virtual void OnMissingModel(const char* modelname) = 0;
	virtual void EnumQueries(IEnumSCModelQueryHandler *handler) = 0;
};

ISCModelDatabase* SCModelDatabase();

#define SCMODEL_DATABASE_INTERFACE_VERSION "SCModelDatabase_API_001"