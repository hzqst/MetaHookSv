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

enum class UtilHTTPField
{
    unknown = 0,
    accept_charset,
    accept_encoding,
    authorization,
    body,
    cache_control,
    content_encoding,
    content_length,
    content_type,
    cookie,
    encoding,
    expires,
    expiry_date,
    host,
    keep_alive,
    location,
    origin,
    path,
    referer,
    set_cookie,
    uri,
    user_agent,
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
    virtual bool GetHeaderSize(const char* name, size_t *buflen) = 0;
    virtual bool GetHeader(const char *name, char *buf, size_t buflen) = 0;
    virtual bool IsResponseCompleted() const = 0;
    virtual bool IsResponseError() const = 0;
    virtual bool IsHeaderReceived() const = 0;
    virtual bool IsStream() const = 0;
};

class IUtilHTTPRequest : public IBaseInterface
{
public:
    virtual void Destroy() = 0;
    virtual void SendAsyncRequest() = 0;
    virtual void SetTimeout(int secs) = 0;
    virtual void SetBody(const char * payload, size_t payloadSize) = 0;
    virtual void SetField(const char * field, const char * value) = 0;
    virtual void SetField(UtilHTTPField field, const char * value) = 0;
    virtual bool IsRequesting() const = 0;
    virtual bool IsResponding() const = 0;
    virtual bool IsRequestSuccessful() const = 0;
    virtual bool IsFinished() const = 0;
    virtual bool IsAsync() const = 0;
    virtual bool IsStream() const = 0;
    virtual UtilHTTPRequestState GetRequestState() const = 0;

    //Only available for SyncRequest
    virtual void WaitForResponse() = 0;
    virtual IUtilHTTPResponse* GetResponse() = 0;

    //Only available for AsyncRequest
    virtual void SetRequestId(UtilHTTPRequestId_t id) = 0;
    virtual UtilHTTPRequestId_t GetRequestId() const = 0;
};


class IUtilHTTPCallbacks : public IBaseInterface
{
public:
    virtual void Destroy() = 0;
    virtual void OnResponseComplete(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse *ResponseInstance) = 0;
    virtual void OnUpdateState(UtilHTTPRequestState NewState) = 0;
};

class IUtilHTTPStreamCallbacks : public IUtilHTTPCallbacks
{
public:
    virtual void OnReceiveHeader(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) = 0;
    virtual void OnReceiveData(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) = 0;
};

class IURLParsedResult : public IBaseInterface
{
public:
    virtual void Destroy() = 0;
    virtual const char *GetScheme() const = 0;
    virtual const char* GetHost() const = 0;
    virtual const char* GetTarget() const = 0;
    virtual const char* GetPortString() const = 0;
    virtual unsigned short GetPort() const = 0;
    virtual bool IsSecure() const = 0;

    virtual void SetScheme(const char* s) = 0;
    virtual void SetHost(const char* s) = 0;
    virtual void SetTarget(const char *s) = 0;
    virtual void SetUsPort(unsigned short p) = 0;
    virtual void SetSecure(bool b) = 0;
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
    virtual void Init(const CUtilHTTPClientCreationContext *context) = 0;
    virtual void Shutdown() = 0;
    virtual void RunFrame() = 0;
    virtual bool ParseUrlEx(const char* url, IURLParsedResult* result) = 0;
    virtual IURLParsedResult* ParseUrl(const char * url) = 0;
    virtual IUtilHTTPRequest* CreateSyncRequest(const char* url, const UtilHTTPMethod method, IUtilHTTPCallbacks* callbacks) = 0;
    virtual IUtilHTTPRequest* CreateAsyncRequest(const char* url, const UtilHTTPMethod method, IUtilHTTPCallbacks* callbacks) = 0;
    virtual IUtilHTTPRequest* GetRequestById(UtilHTTPRequestId_t id) = 0; 
    virtual bool DestroyRequestById(UtilHTTPRequestId_t id) = 0;
    virtual bool SetCookie(const char *host, const char *url, const char* cookie) = 0;
    virtual IUtilHTTPRequest* CreateAsyncStreamRequest(const char* url, const UtilHTTPMethod method, IUtilHTTPStreamCallbacks* callbacks) = 0;
};

#define UTIL_HTTPCLIENT_STEAMAPI_INTERFACE_VERSION "UtilHTTPClient_SteamAPI_004"