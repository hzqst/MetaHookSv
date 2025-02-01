#include <Windows.h>
#include <format>
#include <regex>
#include <mutex>
#include <unordered_map>

#include <IUtilHTTPClient.h>

#include <ScopeExit/ScopeExit.h>

#include <steam_api.h>

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

class CURLParsedResult : public IURLParsedResult
{
public:
	CURLParsedResult(
		const std::string &scheme,
		const std::string& host,
		unsigned short port_us, 
		const std::string& target, 
		bool secure) : m_scheme(scheme), m_host(host), m_port_us(port_us), m_target(target), m_secure(secure)
	{

		m_port_str = std::format("{0}", port_us);
	}

	void Destroy() override
	{
		delete this;
	}

	const char* GetScheme() const override
	{
		return m_scheme.c_str();
	}

	const char* GetHost() const  override
	{
		return m_host.c_str();
	}

	const char* GetTarget() const  override
	{
		return m_target.c_str();
	}

	const char* GetPortString() const  override
	{
		return m_port_str.c_str();
	}

	unsigned short GetPort() const override
	{
		return m_port_us;
	}

	bool IsSecure() const override
	{
		return m_secure;
	}

private:
	std::string m_scheme;
	std::string m_host;
	std::string m_port_str;
	std::string m_target;
	unsigned short m_port_us{};
	bool m_secure{};
};

class CUtilHTTPPayload : public IUtilHTTPPayload
{
private:
	std::string m_payload;

public:

	bool ReadStreamFromRequestHandle(HTTPRequestHandle handle, uint32_t offset, uint32_t size)
	{
		m_payload.resize(size);
		if (SteamHTTP()->GetHTTPStreamingResponseBodyData(handle, offset, (uint8*)m_payload.data(), size))
		{
			return true;
		}

		return false;
	}

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
	bool m_bIsHeaderReceived{};
	bool m_bIsStream{};
	int m_iResponseStatusCode{};
	CUtilHTTPPayload* m_pResponsePayload{};

	HTTPRequestHandle m_RequestHandle{ INVALID_HTTPREQUEST_HANDLE  };
public:

	void OnSteamHTTPRecvHeader(HTTPRequestHeadersReceived_t* pResult, bool bHasError)
	{
		m_bIsHeaderReceived = true;
		m_bIsStream = true;

		m_RequestHandle = pResult->m_hRequest;
	}

	void OnSteamHTTPRecvData(HTTPRequestDataReceived_t* pResult, bool bHasError)
	{
		m_RequestHandle = pResult->m_hRequest;

		if (m_bIsStream && !bHasError)
		{
			m_pResponsePayload->ReadStreamFromRequestHandle(pResult->m_hRequest, pResult->m_cOffset, pResult->m_cBytesReceived);
		}
	}

	void OnSteamHTTPCompleted(HTTPRequestCompleted_t* pResult, bool bHasError)
	{
		m_RequestHandle = pResult->m_hRequest;

		m_bIsHeaderReceived = true;
		m_bResponseCompleted = true;
		m_bResponseError = bHasError;
		m_iResponseStatusCode = (int)pResult->m_eStatusCode;

		if (pResult->m_bRequestSuccessful && !bHasError && !m_bIsStream)
		{
			m_pResponsePayload->ReadFromRequestHandle(pResult->m_hRequest);
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

	bool GetHeaderSize(const char* name, size_t *buflen) override
	{
		return SteamHTTP()->GetHTTPResponseHeaderSize(m_RequestHandle, name, buflen);
	}

	bool GetHeader(const char* name, char* buf, size_t buflen) override
	{
		return SteamHTTP()->GetHTTPResponseHeaderValue(m_RequestHandle, name, (uint8 *)buf, buflen);
	}

	bool IsResponseCompleted() const override
	{
		return m_bResponseCompleted;
	}

	bool IsResponseError() const override
	{
		return m_bResponseError;
	}

	bool IsHeaderReceived() const override
	{
		return m_bIsHeaderReceived;
	}

	bool IsStream() const override
	{
		return m_bIsStream;
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
	UtilHTTPRequestId_t m_RequestId{ UTILHTTP_REQUEST_INVALID_ID };
	HTTPRequestHandle m_RequestHandle{};
	CCallResult<CUtilHTTPRequest, HTTPRequestCompleted_t> m_CompleteCallResult{};
	bool m_bRequesting{};
	bool m_bResponding{};
	bool m_bRequestSuccessful{};
	bool m_bFinished{};
	bool m_bAutoDestroyOnFinish{};
	IUtilHTTPCallbacks* m_Callbacks{};
	CUtilHTTPResponse* m_pResponse{};

public:

	CUtilHTTPRequest(
		const UtilHTTPMethod method,
		const std::string& host,
		unsigned short port,
		bool secure,
		const std::string& target,
		IUtilHTTPCallbacks* callbacks,
		HTTPCookieContainerHandle hCookieHandle) :
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

		SteamHTTP()->SetHTTPRequestCookieContainer(m_RequestHandle, hCookieHandle);

		SetField(UtilHTTPField::host, field_host.c_str());
		SetField(UtilHTTPField::user_agent, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/84.0.4147.105 Safari/537.36");
	}

	~CUtilHTTPRequest()
	{
		if (m_CompleteCallResult.IsActive())
		{
			m_CompleteCallResult.Cancel();
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

	virtual void OnRespondStart()
	{
		if (!m_bResponding)
		{
			m_bRequesting = false;
			m_bResponding = true;

			if (m_Callbacks)
			{
				m_Callbacks->OnUpdateState(UtilHTTPRequestState::Responding);
			}
		}
	}

	virtual void OnRespondFinish()
	{
		if (m_bResponding)
		{
			m_bFinished = true;
			m_bResponding = false;

			if (m_Callbacks)
			{
				m_Callbacks->OnUpdateState(UtilHTTPRequestState::Finished);
			}
		}
	}

	virtual void OnSteamHTTPCompleted(HTTPRequestCompleted_t* pResult, bool bHasError)
	{
		OnRespondStart();

		m_bRequestSuccessful = pResult->m_bRequestSuccessful;

		m_pResponse->OnSteamHTTPCompleted(pResult, bHasError);

		if (m_Callbacks)
		{
			m_Callbacks->OnResponseComplete(this, m_pResponse);
		}

		OnRespondFinish();
	}

public:

	void Destroy() override
	{
		delete this;
	}

	void Send() override
	{
		SteamAPICall_t SteamApiCall;
		if (SteamHTTP()->SendHTTPRequest(m_RequestHandle, &SteamApiCall))
		{
			m_CompleteCallResult.Set(SteamApiCall, this, &CUtilHTTPRequest::OnSteamHTTPCompleted);
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

	bool IsAutoDestroyOnFinish() const override
	{
		return m_bAutoDestroyOnFinish;
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

	void SetRequestId(UtilHTTPRequestId_t id) override
	{
		m_RequestId = id;
	}

	UtilHTTPRequestId_t GetRequestId() const override
	{
		return m_RequestId;
	}

	void SetAutoDestroyOnFinish(bool b) override
	{
		m_bAutoDestroyOnFinish = b;
	}

	void SetTimeout(int secs) override
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
		IUtilHTTPCallbacks* callbacks,
		HTTPCookieContainerHandle hCookieHandle) :
		CUtilHTTPRequest(method, host, port, secure, target, callbacks, hCookieHandle)
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

	bool IsStream() const override
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
};

class CUtilHTTPAsyncRequest : public CUtilHTTPRequest
{
private:

public:
	CUtilHTTPAsyncRequest(
		const UtilHTTPMethod method,
		const std::string& host,
		unsigned short port,
		bool secure,
		const std::string& target,
		IUtilHTTPCallbacks* callbacks,
		HTTPCookieContainerHandle hCookieHandle) :
		CUtilHTTPRequest(method, host, port, secure, target, callbacks, hCookieHandle)
	{
		m_bAutoDestroyOnFinish = true;
	}

	bool IsAsync() const override
	{
		return true;
	}

	bool IsStream() const override
	{
		return false;
	}

	void WaitForResponse() override
	{

	}

	IUtilHTTPResponse* GetResponse() override
	{
		return nullptr;
	}
};

class CUtilHTTPAsyncStreamRequest : public CUtilHTTPAsyncRequest
{
private:
	CCallResult<CUtilHTTPAsyncStreamRequest, HTTPRequestCompleted_t> m_StreamCompleteCallResult{};
	CCallResult<CUtilHTTPAsyncStreamRequest, HTTPRequestHeadersReceived_t> m_RecvHeaderCallResult{};
	CCallResult<CUtilHTTPAsyncStreamRequest, HTTPRequestDataReceived_t> m_RecvDataCallResult{};

	IUtilHTTPStreamCallbacks* m_StreamCallbacks{};

public:
	CUtilHTTPAsyncStreamRequest(
		const UtilHTTPMethod method,
		const std::string& host,
		unsigned short port,
		bool secure,
		const std::string& target,
		IUtilHTTPStreamCallbacks* StreamCallbacks,
		HTTPCookieContainerHandle hCookieHandle) :
		CUtilHTTPAsyncRequest(method, host, port, secure, target, nullptr, hCookieHandle), m_StreamCallbacks(StreamCallbacks)
	{

	}

	~CUtilHTTPAsyncStreamRequest()
	{
		if (m_StreamCompleteCallResult.IsActive())
		{
			m_StreamCompleteCallResult.Cancel();
		}
		if (m_RecvHeaderCallResult.IsActive())
		{
			m_RecvHeaderCallResult.Cancel();
		}
		if (m_RecvDataCallResult.IsActive())
		{
			m_RecvDataCallResult.Cancel();
		}
		if (m_StreamCallbacks)
		{
			m_StreamCallbacks->Destroy();
			m_StreamCallbacks = nullptr;
		}
	}

	bool IsStream() const override
	{
		return true;
	}

	virtual void OnSteamHTTPRecvHeader(HTTPRequestHeadersReceived_t* pResult, bool bHasError)
	{
		OnRespondStart();

		m_pResponse->OnSteamHTTPRecvHeader(pResult, bHasError);

		if (m_StreamCallbacks)
		{
			m_StreamCallbacks->OnReceiveHeader(this, m_pResponse);
		}
	}

	virtual void OnSteamHTTPRecvData(HTTPRequestDataReceived_t* pResult, bool bHasError)
	{
		m_pResponse->OnSteamHTTPRecvData(pResult, bHasError);

		if (m_StreamCallbacks)
		{
			m_StreamCallbacks->OnReceiveData(this, m_pResponse);
		}
	}

	void OnSteamHTTPCompleted(HTTPRequestCompleted_t* pResult, bool bHasError) override
	{
		OnRespondStart();

		m_bRequestSuccessful = pResult->m_bRequestSuccessful;

		m_pResponse->OnSteamHTTPCompleted(pResult, bHasError);

		if (m_StreamCallbacks)
		{
			m_StreamCallbacks->OnResponseComplete(this, m_pResponse);
		}

		OnRespondFinish();
	}

	void OnRespondStart() override
	{
		if (!m_bResponding)
		{
			m_bRequesting = false;
			m_bResponding = true;

			if (m_StreamCallbacks)
			{
				m_StreamCallbacks->OnUpdateState(UtilHTTPRequestState::Responding);
			}
		}
	}

	void OnRespondFinish() override
	{
		if (m_bResponding)
		{
			m_bFinished = true;
			m_bResponding = false;

			if (m_StreamCallbacks)
			{
				m_StreamCallbacks->OnUpdateState(UtilHTTPRequestState::Finished);
			}
		}
	}

	void Send() override
	{
		SteamAPICall_t SteamApiCall;
		if (SteamHTTP()->SendHTTPRequestAndStreamResponse(m_RequestHandle, &SteamApiCall))
		{
			m_StreamCompleteCallResult.Set(SteamApiCall, this, &CUtilHTTPAsyncStreamRequest::OnSteamHTTPCompleted);
			m_RecvHeaderCallResult.Set(SteamApiCall, this, &CUtilHTTPAsyncStreamRequest::OnSteamHTTPRecvHeader);
			m_RecvDataCallResult.Set(SteamApiCall, this, &CUtilHTTPAsyncStreamRequest::OnSteamHTTPRecvData);
		}

		m_bRequesting = true;

		if (m_StreamCallbacks)
		{
			m_StreamCallbacks->OnUpdateState(UtilHTTPRequestState::Requesting);
		}
	}
};

class CUtilHTTPClient : public IUtilHTTPClient
{
private:
	std::mutex m_RequestHandleLock;
	UtilHTTPRequestId_t m_RequestUsedId{ UTILHTTP_REQUEST_START_ID };
	std::unordered_map<UtilHTTPRequestId_t, IUtilHTTPRequest*> m_RequestPool;
	HTTPCookieContainerHandle m_CookieHandle{ INVALID_HTTPCOOKIE_HANDLE  };

public:

	~CUtilHTTPClient()
	{
		if (m_CookieHandle)
		{
			SteamHTTP()->ReleaseCookieContainer(m_CookieHandle);
			m_CookieHandle = INVALID_HTTPCOOKIE_HANDLE;
		}
	}

	void Destroy() override
	{
		delete this;
	}

	void Init(const CUtilHTTPClientCreationContext* context) override
	{
		if (context->m_bUseCookieContainer)
		{
			m_CookieHandle = SteamHTTP()->CreateCookieContainer(context->m_bAllowResponseToModifyCookie);
		}
	}

	void Shutdown() override
	{
		std::lock_guard<std::mutex> lock(m_RequestHandleLock);

		for (auto itor = m_RequestPool.begin(); itor != m_RequestPool.end(); itor ++)
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

			if (RequestInstance->IsFinished() && RequestInstance->IsAutoDestroyOnFinish())
			{
				RequestInstance->Destroy();

				itor = m_RequestPool.erase(itor);

				continue;
			}

			itor++;
		}
	}

	IURLParsedResult* ParseUrlInternal(const std::string& url)
	{
		std::regex url_regex(
			R"((http|https)://([^/]+)(:?(\d+)?)?(/.*)?)",
			std::regex_constants::icase
		);

		std::smatch url_match_result;

		if (std::regex_match(url, url_match_result, url_regex)) {
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

				return new CURLParsedResult(scheme, host, port_us, target, (scheme == "https") ? true : false);
			}
		}

		return nullptr;
	}

	IURLParsedResult *ParseUrl(const char *url) override
	{
		return ParseUrlInternal(url);
	}

	IUtilHTTPRequest* CreateSyncRequestEx(const char* host, unsigned short port_us, const char* target, bool secure, const UtilHTTPMethod method, IUtilHTTPCallbacks* callback)
	{
		return new CUtilHTTPSyncRequest(method, host, port_us, secure, target, callback, m_CookieHandle);
	}

	IUtilHTTPRequest* CreateAsyncRequestEx(const char * host, unsigned short port_us, const char* target, bool secure, const UtilHTTPMethod method, IUtilHTTPCallbacks* callback)
	{
		return new CUtilHTTPAsyncRequest(method, host, port_us, secure, target, callback, m_CookieHandle);
	}

	IUtilHTTPRequest* CreateSyncRequest(const char* url, const UtilHTTPMethod method, IUtilHTTPCallbacks* callbacks) override
	{
		auto result = ParseUrl(url);

		if (!result)
			return nullptr;

		SCOPE_EXIT{ result->Destroy(); };

		return CreateSyncRequestEx(result->GetHost(), result->GetPort(), result->GetTarget(), result->IsSecure(), method, callbacks);
	}

	IUtilHTTPRequest* CreateAsyncRequest(const char* url, const UtilHTTPMethod method, IUtilHTTPCallbacks* callbacks) override
	{
		auto result = ParseUrl(url);

		if (!result)
			return nullptr;

		SCOPE_EXIT{ result->Destroy(); };

		return CreateAsyncRequestEx(result->GetHost(), result->GetPort(), result->GetTarget(), result->IsSecure(), method, callbacks);
	}


	IUtilHTTPRequest* CreateAsyncStreamRequestEx(const char* host, unsigned short port_us, const char* target, bool secure, const UtilHTTPMethod method, IUtilHTTPStreamCallbacks* callback)
	{
		return new CUtilHTTPAsyncStreamRequest(method, host, port_us, secure, target, callback, m_CookieHandle);
	}

	IUtilHTTPRequest* CreateAsyncStreamRequest(const char* url, const UtilHTTPMethod method, IUtilHTTPStreamCallbacks* callbacks) override
	{
		auto result = ParseUrl(url);

		if (!result)
			return nullptr;

		SCOPE_EXIT{ result->Destroy(); };

		return CreateAsyncStreamRequestEx(result->GetHost(), result->GetPort(), result->GetTarget(), result->IsSecure(), method, callbacks);
	}

	void AddToRequestPool(IUtilHTTPRequest* RequestInstance) override
	{
		std::lock_guard<std::mutex> lock(m_RequestHandleLock);

		if (m_RequestUsedId == UTILHTTP_REQUEST_MAX_ID)
			m_RequestUsedId = UTILHTTP_REQUEST_START_ID;

		auto RequestId = m_RequestUsedId;

		RequestInstance->SetRequestId(RequestId);

		m_RequestPool[RequestId] = RequestInstance;

		m_RequestUsedId++;
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
			auto pRequest = itor->second;

			m_RequestPool.erase(itor);

			pRequest->Destroy();

			return true;
		}

		return false;
	}

	bool SetCookie(const char* host, const char* url, const char* cookie) override
	{
		if (m_CookieHandle != INVALID_HTTPCOOKIE_HANDLE)
		{
			return SteamHTTP()->SetCookie(m_CookieHandle, host, url, cookie);
		}

		return false;
	}

};

EXPOSE_INTERFACE(CUtilHTTPClient, IUtilHTTPClient, UTIL_HTTPCLIENT_STEAMAPI_INTERFACE_VERSION);