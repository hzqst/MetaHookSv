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
	bool ReadFromRequestHandle(HTTPRequestHandle handle)
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

class CUtilHTTPResponse : public IUtilHTTPResponse
{
private:
	bool m_bResponseCompleted{};
	bool m_bResponseError{};
	int m_iResponseStatusCode{};
	CUtilHTTPPayload* m_pResponsePayload{};
public:

	void OnSteamHTTPCompleted(HTTPRequestHandle RequestHandle, HTTPRequestCompleted_t* pResult, bool bHasError)
	{
		m_bResponseCompleted = true;
		m_bResponseError = bHasError;
		m_iResponseStatusCode = (int)pResult->m_eStatusCode;

		if (pResult->m_bRequestSuccessful && !bHasError)
		{
			m_pResponsePayload->ReadFromRequestHandle(RequestHandle);
		}
	}

public:
	CUtilHTTPResponse() : m_pResponsePayload(new CUtilHTTPPayload())
	{

	}

	~CUtilHTTPResponse()
	{
		if (m_pResponsePayload)
		{
			delete m_pResponsePayload;
			m_pResponsePayload = nullptr;
		}
	}

	bool IsResponseCompleted() const override
	{
		return m_bResponseCompleted;
	}

	bool IsResponseError() const override
	{
		return m_bResponseError;
	}

	int GetStatusCode() const override
	{
		return m_iResponseStatusCode;
	}

	IUtilHTTPPayload* GetPayload() const override
	{
		return m_pResponsePayload;
	}
};

class CUtilHTTPRequest : public IUtilHTTPRequest
{
protected:
	HTTPRequestHandle m_RequestHandle{};
	CCallResult<CUtilHTTPRequest, HTTPRequestCompleted_t> m_CallResult{};
	bool m_bRequesting{};
	bool m_bResponding{};
	bool m_bRequestSuccessful{};
	bool m_bFinished{};
	IUtilHTTPCallbacks* m_Callbacks{};
	CUtilHTTPResponse* m_pResponse{};

public:

	CUtilHTTPRequest(
		const UtilHTTPMethod method,
		const std::string& host,
		unsigned short port,
		bool secure,
		const std::string& target,
		IUtilHTTPCallbacks* callbacks) : 
		m_Callbacks(callbacks),
		m_pResponse(new CUtilHTTPResponse())
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

		std::string url = std::format("{0}://{1}{2}", secure ? "https" :"http", field_host, target);

		m_RequestHandle = SteamHTTP()->CreateHTTPRequest(UTIL_ConvertUtilHTTPMethodToSteamHTTPMethod(method), url.c_str());

		SetField(UtilHTTPField::host, field_host.c_str());
		SetField(UtilHTTPField::user_agent, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/84.0.4147.105 Safari/537.36");
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

		if (m_Callbacks)
		{
			m_Callbacks->Destroy();
			m_Callbacks = nullptr;
		}
	}

	//TODO stream read?
	virtual void OnSteamHTTPCompleted(HTTPRequestCompleted_t* pResult, bool bHasError)
	{
		m_bRequesting = false;
		m_bResponding = true;

		if (m_Callbacks)
		{
			m_Callbacks->OnUpdateState(UtilHTTPRequestState::Responding);
		}

		m_bRequestSuccessful = pResult->m_bRequestSuccessful;

		m_pResponse->OnSteamHTTPCompleted(m_RequestHandle, pResult, bHasError);

		if (m_Callbacks)
		{
			m_Callbacks->OnResponse(this, m_pResponse);
		}

		m_bFinished = true;
		m_bResponding = false;

		if (m_Callbacks)
		{
			m_Callbacks->OnUpdateState(UtilHTTPRequestState::Finished);
		}
	}

public:

	void Destroy() override
	{
		delete this;
	}

	void SendAsyncRequest() override
	{
		SteamAPICall_t SteamApiCall;
		if (SteamHTTP()->SendHTTPRequest(m_RequestHandle, &SteamApiCall))
		{
			m_CallResult.Set(SteamApiCall, this, &CUtilHTTPRequest::OnSteamHTTPCompleted);
		}

		m_bRequesting = true;

		if (m_Callbacks)
		{
			m_Callbacks->OnUpdateState(UtilHTTPRequestState::Requesting);
		}
	}

	bool IsRequesting() const override
	{
		return m_bRequesting;
	}

	bool IsResponding() const override
	{
		return m_bResponding;
	}

	bool IsRequestSuccessful() const override
	{
		return m_bRequestSuccessful;
	}

	bool IsFinished() const override
	{
		return m_bFinished;
	}

	UtilHTTPRequestState GetRequestState() const override
	{
		if (IsFinished())
			return UtilHTTPRequestState::Finished;

		if (IsRequesting())
			return UtilHTTPRequestState::Requesting;

		if (IsResponding())
			return UtilHTTPRequestState::Responding;

		return UtilHTTPRequestState::Idle;
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
};

class CUtilHTTPSyncRequest : public CUtilHTTPRequest
{
private:
	HANDLE m_hResponseEvent{};

public:
	CUtilHTTPSyncRequest(
		const UtilHTTPMethod method,
		const std::string& host,
		unsigned short port,
		bool secure,
		const std::string& target,
		IUtilHTTPCallbacks* callbacks) : 
		CUtilHTTPRequest(method, host, port, secure, target, callbacks)
	{
		m_hResponseEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
	}

	~CUtilHTTPSyncRequest()
	{
		if (m_hResponseEvent)
		{
			CloseHandle(m_hResponseEvent);
			m_hResponseEvent = NULL;
		}

	}

	bool IsAsync() const override
	{
		return false;
	}

	void WaitForResponse() override
	{
		if (m_hResponseEvent)
		{
			WaitForSingleObject(m_hResponseEvent, INFINITE);
		}
	}

	IUtilHTTPResponse* GetResponse() override
	{
		return m_pResponse;
	}

	void OnSteamHTTPCompleted(HTTPRequestCompleted_t* pResult, bool bHasError) override
	{
		CUtilHTTPRequest::OnSteamHTTPCompleted(pResult, bHasError);

		if (m_hResponseEvent)
		{
			SetEvent(m_hResponseEvent);
		}
	}

	void SetRequestId(UtilHTTPRequestId_t id) override
	{
		
	}

	UtilHTTPRequestId_t GetRequestId() const override
	{
		return UTILHTTP_REQUEST_INVALID_ID;
	}
};

class CUtilHTTPAsyncRequest : public CUtilHTTPRequest
{
private:
	UtilHTTPRequestId_t m_RequestId{ UTILHTTP_REQUEST_INVALID_ID };

public:
	CUtilHTTPAsyncRequest(
		const UtilHTTPMethod method,
		const std::string& host,
		unsigned short port,
		bool secure,
		const std::string& target,
		IUtilHTTPCallbacks* callbacks) :
		CUtilHTTPRequest(method, host, port, secure, target, callbacks)
	{

	}

	bool IsAsync() const override
	{
		return true;
	}

	void WaitForResponse() override
	{

	}

	IUtilHTTPResponse* GetResponse() override
	{
		return nullptr;
	}

	void SetRequestId(UtilHTTPRequestId_t id) override
	{
		m_RequestId = id;
	}

	UtilHTTPRequestId_t GetRequestId() const override
	{
		return m_RequestId;
	}
};

class CUtilHTTPClient : public IUtilHTTPClient
{
private:
	std::mutex m_RequestHandleLock;
	UtilHTTPRequestId_t m_RequestUsedId{ UTILHTTP_REQUEST_START_ID };
	std::unordered_map<UtilHTTPRequestId_t, IUtilHTTPRequest*> m_RequestPool;

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
			auto RequestInstance = (*itor).second;

			RequestInstance->Destroy();
		}

		m_RequestPool.clear();
	}

	void RunFrame() override
	{
		std::lock_guard<std::mutex> lock(m_RequestHandleLock);

		for (auto itor = m_RequestPool.begin(); itor != m_RequestPool.end();)
		{
			auto RequestInstance = (*itor).second;

			if (RequestInstance->IsFinished())
			{
				RequestInstance->Destroy();

				itor = m_RequestPool.erase(itor);

				continue;
			}

			itor++;
		}
	}

	void AddToRequestPool(IUtilHTTPRequest *RequestInstance)
	{
		std::lock_guard<std::mutex> lock(m_RequestHandleLock);

		if (m_RequestUsedId == UTILHTTP_REQUEST_MAX_ID)
			m_RequestUsedId = UTILHTTP_REQUEST_START_ID;

		auto RequestId = m_RequestUsedId;

		RequestInstance->SetRequestId(RequestId);

		m_RequestPool[RequestId] = RequestInstance;

		m_RequestUsedId++;
	}

	bool ParseUrlEx(const char* url, IURLParsedResult *result) override
	{
		std::string surl = url;

		std::regex url_regex(
			R"((http|https)://([^/]+)(:?(\d+)?)?(/.*)?)",
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
				std::string target = (url_match_result.size() >= 6) ? url_match_result[5].str() : "";

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

	IUtilHTTPRequest* CreateSyncRequestEx(const char* host, unsigned short port_us, const char* target, bool secure, const UtilHTTPMethod method, IUtilHTTPCallbacks* callback)
	{
		return new CUtilHTTPSyncRequest(method, host, port_us, secure, target, callback);
	}

	IUtilHTTPRequest* CreateAsyncRequestEx(const char * host, unsigned short port_us, const char* target, bool secure, const UtilHTTPMethod method, IUtilHTTPCallbacks* callback)
	{
		return new CUtilHTTPAsyncRequest(method, host, port_us, secure, target, callback);
	}

	IUtilHTTPRequest* CreateSyncRequest(const char* url, const UtilHTTPMethod method, IUtilHTTPCallbacks* callbacks) override
	{
		CURLParsedResult result;

		if (!ParseUrlEx(url, &result))
			return NULL;

		return CreateSyncRequestEx(result.GetHost(), result.GetPort(), result.GetTarget(), result.IsSecure(), method, callbacks);
	}

	IUtilHTTPRequest* CreateAsyncRequest(const char* url, const UtilHTTPMethod method, IUtilHTTPCallbacks* callbacks) override
	{
		CURLParsedResult result;

		if (!ParseUrlEx(url, &result))
			return NULL;

		auto RequestInstance = CreateAsyncRequestEx(result.GetHost(), result.GetPort(), result.GetTarget(), result.IsSecure(), method, callbacks);

		AddToRequestPool(RequestInstance);

		return RequestInstance;
	}

	IUtilHTTPRequest* GetRequestById(UtilHTTPRequestId_t id) override
	{
		std::lock_guard<std::mutex> lock(m_RequestHandleLock);

		auto itor = m_RequestPool.find(id);
		if (itor != m_RequestPool.end())
		{
			return itor->second;
		}

		return NULL;
	}

	bool DestroyRequestById(UtilHTTPRequestId_t id) override
	{
		std::lock_guard<std::mutex> lock(m_RequestHandleLock);

		auto itor = m_RequestPool.find(id);
		if (itor != m_RequestPool.end())
		{
			delete itor->second;

			m_RequestPool.erase(itor);

			return true;
		}

		return false;
	}
};

EXPOSE_SINGLE_INTERFACE(CUtilHTTPClient, IUtilHTTPClient, UTIL_HTTPCLIENT_STEAMAPI_INTERFACE_VERSION);