#include "IUtilHTTPClient.h"

#include <Windows.h>
#include <format>
#include <regex>
#include <mutex>
#include <unordered_map>

#include <steam_api.h>

/*
enum EHTTPMethod
{
	k_EHTTPMethodInvalid = 0,
	k_EHTTPMethodGET,
	k_EHTTPMethodHEAD,
	k_EHTTPMethodPOST,
	k_EHTTPMethodPUT,
	k_EHTTPMethodDELETE,
	k_EHTTPMethodOPTIONS,
	k_EHTTPMethodPATCH,
};
*/

EHTTPMethod UTIL_ConvertUtilHTTPMethodToSteamHTTPMethod(const UtilHTTPMethod method)
{
	switch (method)
	{
	case UtilHTTPMethod::Get:
		return EHTTPMethod::k_EHTTPMethodGET;
	case UtilHTTPMethod::Head:
		return EHTTPMethod::k_EHTTPMethodHEAD;
	case UtilHTTPMethod::Post:
		return EHTTPMethod::k_EHTTPMethodPOST;
	case UtilHTTPMethod::Put:
		return EHTTPMethod::k_EHTTPMethodPUT;
	case UtilHTTPMethod::Delete:
		return EHTTPMethod::k_EHTTPMethodDELETE;
	}

	return EHTTPMethod::k_EHTTPMethodInvalid;
}

class CUtilHTTPRequest;

class CURLParsedResult : public IURLParsedResult
{
public:
	void Destroy() override
	{
		delete this;
	}

	const char* GetScheme() const override
	{
		return scheme.c_str();
	}

	const char* GetHost() const  override
	{
		return host.c_str();
	}

	const char* GetTarget() const  override
	{
		return target.c_str();
	}

	const char* GetPortString() const  override
	{
		return port_str.c_str();
	}

	unsigned short GetPort() const override
	{
		return port_us;
	}

	bool IsSecure() const override
	{
		return secure;
	}

	void SetScheme(const char* s) override
	{
		scheme = s;
	}

	void SetHost(const char* s) override
	{
		host = s;
	}

	void SetTarget(const char* s) override
	{
		target = s;
	}

	void SetUsPort(unsigned short p) override
	{
		port_us = p;
		port_str = std::format("{0}", p);
	}

	void SetSecure(bool b) override
	{
		secure = b;
	}

private:
	std::string scheme;
	std::string host;
	std::string port_str;
	std::string target;
	unsigned short port_us{};
	bool secure{};
};

class CUtilHTTPPayload : public IUtilHTTPPayload
{
private:
	std::string m_payload;

public:
	bool ReadFromSteamHTTPRequestHandle(HTTPRequestHandle handle)
	{
		uint32_t responseSize = 0;
		if (SteamHTTP()->GetHTTPResponseBodySize(handle, &responseSize))
		{
			m_payload.resize(responseSize);
			if (SteamHTTP()->GetHTTPResponseBodyData(handle, (uint8*)m_payload.data(), responseSize))
			{
				return true;
			}
		}

		return false;
	}

	const char* GetBytes() const
	{
		return m_payload.data();
	}

	size_t GetLength() const
	{
		return m_payload.size();
	}
};

class CUtilHTTPRequest : public IUtilHTTPRequest, public IUtilHTTPResponse
{
private:
	HTTPRequestHandle m_RequestHandle{};
	CCallResult<CUtilHTTPRequest, HTTPRequestCompleted_t> m_CallResult{};
	HANDLE m_hResponseEvent{};
	bool m_bRequestSuccessful{};
	bool m_bResponseCompleted{};
	bool m_bResponseError{};
	int m_iResponseStatusCode{};
	CUtilHTTPPayload* m_pResponsePayload{};
	IUtilHTTPCallbacks* m_Callbacks{};

public:
	CUtilHTTPRequest(
		const UtilHTTPMethod method,
		const std::string& host,
		unsigned short port,
		bool secure,
		const std::string& target,
		IUtilHTTPCallbacks *callbacks) : m_Callbacks(callbacks), m_pResponsePayload(new CUtilHTTPPayload)
	{
		std::string field_host = host;

		if (secure && port != 443)
		{
			field_host = std::format("{0}:{1}", host, port);
		}
		else if (!secure && port != 80)
		{
			field_host = std::format("{0}:{1}", host, port);
		}

		std::string url = std::format("{0}://{1}/{2}", secure ? "https://" :"http://", field_host, target);

		m_RequestHandle = SteamHTTP()->CreateHTTPRequest(UTIL_ConvertUtilHTTPMethodToSteamHTTPMethod(method), url.c_str());

		SetField(UtilHTTPField::host, field_host.c_str());
		SetField(UtilHTTPField::user_agent, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/84.0.4147.105 Safari/537.36");

		m_hResponseEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
	}

	~CUtilHTTPRequest()
	{
		if (m_CallResult.IsActive())
		{
			m_CallResult.Cancel();
		}

		if (m_RequestHandle != INVALID_HTTPREQUEST_HANDLE)
		{
			SteamHTTP()->ReleaseHTTPRequest(m_RequestHandle);
			m_RequestHandle = INVALID_HTTPREQUEST_HANDLE;
		}

		if (m_hResponseEvent)
		{
			CloseHandle(m_hResponseEvent);
			m_hResponseEvent = NULL;
		}

		if (m_pResponsePayload)
		{
			delete m_pResponsePayload;
			m_pResponsePayload = nullptr;
		}

		if (m_Callbacks)
		{
			m_Callbacks->Destroy();
			m_Callbacks = nullptr;
		}

	}

	void OnSteamHTTPCompleted(HTTPRequestCompleted_t* pResult, bool bHasError)
	{
		m_bRequestSuccessful = pResult->m_bRequestSuccessful;
		m_bResponseCompleted = true;
		m_bResponseError = bHasError;
		m_iResponseStatusCode = (int)pResult->m_eStatusCode;
		
		if (m_bRequestSuccessful && !m_bResponseError && m_pResponsePayload)
		{
			m_pResponsePayload->ReadFromSteamHTTPRequestHandle(m_RequestHandle);
		}

		if (m_Callbacks)
		{
			m_Callbacks->OnResponse(this);
		}

		if (m_hResponseEvent)
		{
			SetEvent(m_hResponseEvent);
		}
	}

	void SendAsyncRequest()
	{
		if (m_Callbacks)
		{
			m_Callbacks->OnRequest(this);
		}

		SteamAPICall_t apiCall;
		bool bRet = SteamHTTP()->SendHTTPRequest(m_RequestHandle, &apiCall);

		if (bRet)
		{
			m_CallResult.Set(apiCall, this, &CUtilHTTPRequest::OnSteamHTTPCompleted);
		}
	}

	bool SyncWaitForResponse()
	{
		if (m_hResponseEvent)
		{
			WaitForSingleObject(m_hResponseEvent, INFINITE);
		}

		return (m_bResponseCompleted && !m_bResponseError) ? true : false;
	}

public:

	void ForceShutdown() override
	{
		//has nothing to do
	}

	bool IsRequestSuccessful() const override
	{
		return m_bRequestSuccessful;
	}

	bool IsResponseCompleted() const override
	{
		return m_bResponseCompleted;
	}

	bool IsResponseError() const override
	{
		return m_bResponseError;
	}

	void SetTimeout(int secs)  override
	{
		SteamHTTP()->SetHTTPRequestNetworkActivityTimeout(m_RequestHandle, secs);
	}

	void SetBody(const char *payload, size_t payloadSize) override
	{
		//You can change content-type later
		SteamHTTP()->SetHTTPRequestRawPostBody(m_RequestHandle, "application/octet-stream", (uint8_t *)payload, payloadSize);
	}

	void SetField(const char* field, const char* value) override
	{
		SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, field, value);
	}

	void SetField(UtilHTTPField field, const char* value) override
	{
		switch (field)
		{
		case UtilHTTPField::user_agent:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "User-Agent", value);
			break;
		case UtilHTTPField::uri:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Uri", value);
			break;
		case UtilHTTPField::set_cookie:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Set-Cookie", value);
			break;
		case UtilHTTPField::referer:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Referer", value);
			break;
		case UtilHTTPField::path:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Path", value);
			break;
		case UtilHTTPField::origin:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Origin", value);
			break;
		case UtilHTTPField::location:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Location", value);
			break;
		case UtilHTTPField::keep_alive:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Keep-Alive", value);
			break;
		case UtilHTTPField::host:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Host", value);
			break;
		case UtilHTTPField::expires:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Expires", value);
			break;
		case UtilHTTPField::expiry_date:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Expiry-Date", value);
			break;
		case UtilHTTPField::encoding:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Encoding", value);
			break;
		case UtilHTTPField::cookie:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Cookie", value);
			break;
		case UtilHTTPField::content_type:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Content-Type", value);
			break;
		case UtilHTTPField::content_length:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Content-Length", value);
			break;
		case UtilHTTPField::content_encoding:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Content-Encoding", value);
			break;
		case UtilHTTPField::cache_control:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Cache-Control", value);
			break;
		case UtilHTTPField::body:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Cache-Body", value);
			break;
		case UtilHTTPField::authorization:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Authorization", value);
			break;
		case UtilHTTPField::accept_encoding:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Accept-Encoding", value);
			break;
		case UtilHTTPField::accept_charset:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Accept-Charset", value);
			break;
		default:
			break;
		}

	}

	int GetStatusCode() const override
	{
		return m_iResponseStatusCode;
	}

	IUtilHTTPPayload *GetPayload() const override
	{
		return m_pResponsePayload;
	}
};

class CUtilHTTPClient : public IUtilHTTPClient
{
private:
	std::mutex m_RequestHandleLock;
	std::vector<CUtilHTTPRequest*> m_RequestPool;

public:
	CUtilHTTPClient()
	{
	
	}

	void Init() override
	{

	}

	void Shutdown() override
	{
		std::lock_guard<std::mutex> lock(m_RequestHandleLock);

		for (auto itor = m_RequestPool.begin(); itor != m_RequestPool.end();)
		{
			auto RequestInstance = (*itor);

			RequestInstance->ForceShutdown();

			delete RequestInstance;

			itor = m_RequestPool.erase(itor);
			continue;
		}
	}

	void RunFrame() override
	{
		std::lock_guard<std::mutex> lock(m_RequestHandleLock);

		for (auto itor = m_RequestPool.begin(); itor != m_RequestPool.end();)
		{
			auto RequestInstance = (*itor);

			if (RequestInstance->IsResponseCompleted())
			{
				delete RequestInstance;
				itor = m_RequestPool.erase(itor);
				continue;
			}

			itor++;
		}
	}

	void AddRequestToPool(CUtilHTTPRequest *RequestInstance)
	{
		std::lock_guard<std::mutex> lock(m_RequestHandleLock);

		m_RequestPool.emplace_back(RequestInstance);
	}

	bool ParseUrlEx(const char* url, IURLParsedResult *result) override
	{
		std::string surl = url;

		std::regex url_regex(
			R"((http|https)://([-A-Z0-9+&@#/%?=~_|!:,.;]*[-A-Z0-9+&@#/%=~_|]):?(\d+)?([-A-Z0-9+&@#/%?=~_|!:,.;]*)?)",
			std::regex_constants::icase
		);

		std::smatch url_match_result;

		if (std::regex_match(surl, url_match_result, url_regex)) {
			// If we found a match
			if (url_match_result.size() >= 4) {
				// Extract the matched groups
				std::string scheme = url_match_result[1].str();
				std::string host = url_match_result[2].str();
				std::string port_str = url_match_result[3].str();
				std::string target = (url_match_result.size() >= 5) ? url_match_result[4].str() : "";

				unsigned port_us = 0;

				if (!port_str.empty()) {
					port_us = std::stoi(port_str);
				}
				else {
					if (scheme == "http") {
						port_us = 80;
					}
					else if (scheme == "https") {
						port_us = 443;
					}
				}

				result->SetScheme(scheme.c_str());
				result->SetHost(host.c_str());
				result->SetUsPort(port_us);
				result->SetTarget(target.c_str());
				result->SetSecure(false);

				if (scheme == "https") {
					result->SetSecure(true);
				}

				return true;
			}
		}

		return false;
	}

	IURLParsedResult *ParseUrl(const char *url) override
	{
		auto result = new CURLParsedResult;

		if (ParseUrlEx(url, result))
			return result;

		delete result;
		return nullptr;
	}

	bool SyncRequest(const std::string& host, unsigned short port_us, const std::string& target, const UtilHTTPMethod method, IUtilHTTPCallbacks *callback)
	{
		CUtilHTTPRequest RequestInstance(method, host, port_us, false, target, callback);

		RequestInstance.SendAsyncRequest();

		return RequestInstance.SyncWaitForResponse();
	}

	bool SyncRequestTLS(const std::string& host, unsigned short port_us, const std::string& target, const UtilHTTPMethod method, IUtilHTTPCallbacks* callback)
	{
		CUtilHTTPRequest RequestInstance(method, host, port_us, true, target, callback);

		RequestInstance.SendAsyncRequest();

		return RequestInstance.SyncWaitForResponse();
	}

	bool Get(const char * url, IUtilHTTPCallbacks* callback) override
	{
		CURLParsedResult result;

		if (!ParseUrlEx(url, &result))
			return false;

		if (result.IsSecure())
		{
			return SyncRequestTLS(result.GetHost(), result.GetPort(), result.GetTarget(), UtilHTTPMethod::Get, callback);
		}

		return SyncRequest(result.GetHost(), result.GetPort(), result.GetTarget(), UtilHTTPMethod::Get, callback);
	}

	bool Post(const char* url, IUtilHTTPCallbacks* callback) override
	{
		CURLParsedResult result;

		if (!ParseUrlEx(url, &result))
			return false;

		if (result.IsSecure())
		{
			return SyncRequestTLS(result.GetHost(), result.GetPort(), result.GetTarget(), UtilHTTPMethod::Post, callback);
		}

		return SyncRequest(result.GetHost(), result.GetPort(), result.GetTarget(), UtilHTTPMethod::Post, callback);
	}

	void AsyncRequest(const char * host, unsigned short port_us, const char* target, const UtilHTTPMethod method, IUtilHTTPCallbacks* callback)
	{
		auto RequestInstance = new CUtilHTTPRequest(method, host, port_us, false, target, callback);
		
		AddRequestToPool(RequestInstance);

		RequestInstance->SendAsyncRequest();
	}

	void AsyncRequestTLS(const char* host, unsigned short port_us, const char* target, const UtilHTTPMethod method, IUtilHTTPCallbacks* callbacks)
	{
		auto RequestInstance = new CUtilHTTPRequest(method, host, port_us, true, target, callbacks);

		AddRequestToPool(RequestInstance);

		RequestInstance->SendAsyncRequest();
	}

	void AsyncGet(const char* url, IUtilHTTPCallbacks* callbacks) override
	{
		CURLParsedResult result;

		if (!ParseUrlEx(url, &result))
			return;

		if (result.IsSecure())
		{
			return AsyncRequestTLS(result.GetHost(), result.GetPort(), result.GetTarget(), UtilHTTPMethod::Get, callbacks);
		}

		return AsyncRequestTLS(result.GetHost(), result.GetPort(), result.GetTarget(), UtilHTTPMethod::Get, callbacks);
	}

	void AsyncPost(const char* url, IUtilHTTPCallbacks* callbacks) override
	{
		CURLParsedResult result;

		if (!ParseUrlEx(url, &result))
			return;

		if (result.IsSecure())
		{
			return AsyncRequestTLS(result.GetHost(), result.GetPort(), result.GetTarget(), UtilHTTPMethod::Post, callbacks);
		}

		return AsyncRequestTLS(result.GetHost(), result.GetPort(), result.GetTarget(), UtilHTTPMethod::Post, callbacks);
	}

};

EXPOSE_SINGLE_INTERFACE(CUtilHTTPClient, IUtilHTTPClient, UTIL_HTTPCLIENT_STEAMAPI_INTERFACE_VERSION);