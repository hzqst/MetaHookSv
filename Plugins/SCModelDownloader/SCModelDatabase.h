#pragma once

#include <interface.h>

enum SCModelQueryState
{
	SCModelQueryState_Unknown = 0,
	SCModelQueryState_Querying,
	SCModelQueryState_Receiving,
	SCModelQueryState_Failed,
	SCModelQueryState_Finished,
};

class ISCModelQuery : public IBaseInterface
{
public:
	virtual const char* GetName() const = 0;
	virtual const char* GetIdentifier() const = 0;
	virtual const char* GetUrl() const = 0;
	virtual float GetProgress() const = 0;
	virtual bool IsFailed() const = 0;
	virtual bool IsFinished() const = 0;
	virtual bool NeedRetry() const = 0;
	virtual SCModelQueryState GetState() const = 0; 
	virtual unsigned int GetTaskId() const = 0;
};

class IEnumSCModelQueryHandler : public IBaseInterface
{
public:
	virtual void OnEnumQuery(ISCModelQuery* pQuery) = 0;
};

class ISCModelQueryStateChangeHandler : public IBaseInterface
{
public:
	virtual void OnQueryStateChanged(ISCModelQuery* pQuery, SCModelQueryState newState) = 0;
};

class ISCModelDatabase : public IBaseInterface
{
public:
	virtual void Init() = 0;
	virtual void Shutdown() = 0;
	virtual void RunFrame() = 0;
	virtual void QueryModel(const char* modelname) = 0;
	virtual void EnumQueries(IEnumSCModelQueryHandler *handler) = 0;
	virtual void RegisterQueryStateChangeCallback(ISCModelQueryStateChangeHandler* handler) = 0;
	virtual void UnregisterQueryStateChangeCallback(ISCModelQueryStateChangeHandler* handler) = 0;
};

ISCModelDatabase* SCModelDatabase();

#define SCMODEL_DATABASE_INTERFACE_VERSION "SCModelDatabase_API_001"