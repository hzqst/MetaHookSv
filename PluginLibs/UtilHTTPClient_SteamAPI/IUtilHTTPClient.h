#pragma once

#include <interface.h>
#include <stdint.h>
#include <string>
#include <functional>
#include <system_error>

typedef uint64_t UtilHTTPRequestHandle_t;

#define UTILHTTP_INVALID_REQUESTHANDLE (UtilHTTPRequestHandle_t)(0)
#define UTILHTTP_START_REQUESTHANDLE (UtilHTTPRequestHandle_t)(1)
#define UTILHTTP_MAX_REQUESTHANDLE (UtilHTTPRequestHandle_t)(0xffffffffffffffffull)

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

    virtual void SetBody(const std::string& payload) = 0;

    virtual void SetField(const std::string& field, const std::string& value) = 0;

    virtual void SetField(UtilHTTPField field, const std::string& value) = 0;

    virtual bool IsRequestSuccessful() const = 0;
};

class IUtilHTTPResponse : public IBaseInterface
{
public:
    virtual int GetStatusCode() const = 0;

    virtual bool GetBody(std::string& body) const = 0;

    virtual bool IsResponseCompleted() const = 0;
};

using fnHTTPRequestCallback = std::function<void(IUtilHTTPRequest* RequestInstance)>;

using fnHTTPResponseCallback = std::function<void(IUtilHTTPResponse* ResponseInstance)>;

using fnHTTPErrorCallback = std::function<void(const std::error_code& ec)>;

class CURLParsedResult
{
public:
    std::string scheme;
    std::string host;
    std::string port_str;
    std::string target;
    unsigned short port_us;
    bool secure;
};

class IUtilHTTPClient : public IBaseInterface
{
public:
    virtual void Init() = 0;

    virtual void Shutdown() = 0;

    virtual void RunFrame() = 0;

    virtual bool ParseUrl(const std::string& url, CURLParsedResult& result) = 0;

    virtual bool Get(const std::string& url, const fnHTTPRequestCallback& request_callback, const fnHTTPResponseCallback& response_callback, const fnHTTPErrorCallback& error_callback) = 0;

    virtual bool Post(const std::string& url, const std::string& payload, const fnHTTPRequestCallback& request_callback, const fnHTTPResponseCallback& response_callback, const fnHTTPErrorCallback& error_callback) = 0;

    virtual UtilHTTPRequestHandle_t AsyncGet(const std::string& url, const fnHTTPRequestCallback& request_callback, const fnHTTPResponseCallback& response_callback, const fnHTTPErrorCallback& error_callback) = 0;

    virtual UtilHTTPRequestHandle_t AsyncPost(const std::string& url, const std::string& payload, const fnHTTPRequestCallback& request_callback, const fnHTTPResponseCallback& response_callback, const fnHTTPErrorCallback& error_callback) = 0;

};

IUtilHTTPClient* UtilHTTPClient();

#define UTIL_HTTPCLIENT_STEAMAPI_INTERFACE_VERSION "UTIL_HTTPClient_SteamAPI_001"