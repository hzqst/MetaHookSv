#pragma once

#include <interface.h>
#include <stdint.h>
#include <functional>
#include <system_error>

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

class IUtilHTTPRequest : public IBaseInterface
{
public:
    virtual void ForceShutdown() = 0;

    virtual void SetTimeout(int secs) = 0;

    virtual void SetBody(const char * payload, size_t payloadSize) = 0;

    virtual void SetField(const char * field, const char * value) = 0;

    virtual void SetField(UtilHTTPField field, const char * value) = 0;

    virtual bool IsRequestSuccessful() const = 0;
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
    virtual IUtilHTTPPayload *GetPayload() const = 0;
    virtual bool IsResponseCompleted() const = 0;
    virtual bool IsResponseError() const = 0;
};

class IUtilHTTPCallbacks : public IBaseInterface
{
public:
    virtual void Destroy() = 0;
    virtual void OnRequest(IUtilHTTPRequest *RequestInstance) = 0;
    virtual void OnResponse(IUtilHTTPResponse *ResponseInstance) = 0;
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

class IUtilHTTPClient : public IBaseInterface
{
public:
    virtual void Init() = 0;

    virtual void Shutdown() = 0;

    virtual void RunFrame() = 0;

    virtual bool ParseUrlEx(const char* url, IURLParsedResult* result) = 0;

    virtual IURLParsedResult *ParseUrl(const char * url) = 0;

    virtual bool Get(const char * url, IUtilHTTPCallbacks *callbacks) = 0;

    virtual bool Post(const char * url, IUtilHTTPCallbacks *callbacks) = 0;

    virtual void AsyncGet(const char * url, IUtilHTTPCallbacks *callbacks) = 0;

    virtual void AsyncPost(const char * url, IUtilHTTPCallbacks *callbacks) = 0;

};

IUtilHTTPClient* UtilHTTPClient();

#define UTIL_HTTPCLIENT_STEAMAPI_INTERFACE_VERSION "UTIL_HTTPClient_SteamAPI_001"