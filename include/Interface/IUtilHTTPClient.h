#pragma once

#include <interface.h>
#include <stdint.h>

typedef uint32_t UtilHTTPRequestId_t;

#define UTILHTTP_REQUEST_INVALID_ID (UtilHTTPRequestId_t)(0)
#define UTILHTTP_REQUEST_START_ID (UtilHTTPRequestId_t)(1)
#define UTILHTTP_REQUEST_MAX_ID (UtilHTTPRequestId_t)(0xffffffff)

enum class UtilHTTPMethod
{
	Unknown = 0,
	Delete,
	Get,
	Head,
	Post,
	Put,
};

enum class UtilHTTPRequestState
{
	Idle = 0,
	Requesting,
	Responding,
	Finished
};

class IUtilHTTPPayload : public IBaseInterface
{
public:
	virtual const char* GetBytes() const = 0;
	virtual size_t GetLength() const = 0;
};

class IUtilHTTPResponse : public IBaseInterface
{
public:
	virtual int GetStatusCode() const = 0;
	virtual IUtilHTTPPayload* GetPayload() const = 0;
	virtual bool GetHeaderSize(const char* name, size_t* buflen) = 0;
	virtual bool GetHeader(const char* name, char* buf, size_t buflen) = 0;
	virtual bool IsResponseCompleted() const = 0;
	virtual bool IsResponseError() const = 0;
};

class IUtilHTTPRequest : public IBaseInterface
{
public:
	virtual void Destroy() = 0;

	//You should call this to actually send the network request. otherwise nothing happens
	virtual void Send() = 0;

	virtual void SetTimeout(int secs) = 0;
	virtual void SetPostBody(const char* contentType, const char* payload, size_t payloadSize) = 0;
	virtual void SetField(const char* field, const char* value) = 0;

	//Async request is auto-destroy-on-finish by default.
	virtual void SetAutoDestroyOnFinish(bool) = 0;

	virtual bool IsRequesting() const = 0;
	virtual bool IsResponding() const = 0;
	virtual bool IsRequestSuccessful() const = 0;
	virtual bool IsFinished() const = 0;
	virtual bool IsAutoDestroyOnFinish() const = 0;
	virtual bool IsAsync() const = 0;
	virtual bool IsStream() const = 0;
	virtual UtilHTTPRequestState GetRequestState() const = 0;

	virtual void SetRequestId(UtilHTTPRequestId_t id) = 0;
	virtual UtilHTTPRequestId_t GetRequestId() const = 0;

	//Only available for SyncRequest
	virtual void WaitForResponse() = 0;
	virtual IUtilHTTPResponse* GetResponse() = 0;
};

class IUtilHTTPCallbacks : public IBaseInterface
{
public:
	virtual void Destroy() = 0;
	virtual void OnResponseComplete(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) = 0;
	virtual void OnUpdateState(UtilHTTPRequestState NewState) = 0;
};

class IUtilHTTPStreamCallbacks : public IUtilHTTPCallbacks
{
public:
	//Called when receive chunked payload data
	virtual void OnReceiveData(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, const void *pData, size_t cbSize) = 0;
};

class IURLParsedResult : public IBaseInterface
{
public:
	virtual void Destroy() = 0;
	virtual const char* GetScheme() const = 0;
	virtual const char* GetHost() const = 0;
	virtual const char* GetTarget() const = 0;
	virtual const char* GetPortString() const = 0;
	virtual unsigned short GetPort() const = 0;
	virtual bool IsSecure() const = 0;
};

class CUtilHTTPClientCreationContext
{
public:
	bool m_bUseCookieContainer{};
	bool m_bAllowResponseToModifyCookie{};
};

class IUtilHTTPClient : public IBaseInterface
{
public:
	virtual void Destroy() = 0;

	virtual void Init(const CUtilHTTPClientCreationContext* context) = 0;

	virtual void Shutdown() = 0;

	//You should call this function every frame
	virtual void RunFrame() = 0;

	//You are responsible for destroying the IURLParsedResult* instance
	virtual IURLParsedResult* ParseUrl(const char* url) = 0;

	//You should immediately call Destroy() after using the IUtilHTTPRequest* instance
	virtual IUtilHTTPRequest* CreateSyncRequest(const char* url, const UtilHTTPMethod method, IUtilHTTPCallbacks* callbacks) = 0;
	
	//You should never keep IUtilHTTPRequest* pointer between frames, instead you should use the request id to retrieve the request instance
	virtual IUtilHTTPRequest* CreateAsyncRequest(const char* url, const UtilHTTPMethod method, IUtilHTTPCallbacks* callbacks) = 0;
	
	virtual IUtilHTTPRequest* CreateAsyncStreamRequest(const char* url, const UtilHTTPMethod method, IUtilHTTPStreamCallbacks* callbacks) = 0;

	//You will have to either add async request instance to the request pool, or manage the request instance by yourself (e.g. Destroy after finish).
	virtual void AddToRequestPool(IUtilHTTPRequest* RequestInstance) = 0;

	virtual IUtilHTTPRequest* GetRequestById(UtilHTTPRequestId_t id) = 0;

	virtual bool DestroyRequestById(UtilHTTPRequestId_t id) = 0;
	
	virtual bool SetCookie(const char* host, const char* url, const char* cookie) = 0;
	
};

#define UTIL_HTTPCLIENT_STEAMAPI_INTERFACE_VERSION "UtilHTTPClient_SteamAPI_005"
#define UTIL_HTTPCLIENT_LIBCURL_INTERFACE_VERSION "UtilHTTPClient_libcurl_005"