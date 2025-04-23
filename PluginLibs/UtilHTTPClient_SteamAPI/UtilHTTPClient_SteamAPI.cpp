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

	const char* GetBytes() const
	{
		return m_payload.data();
	}

	size_t GetLength() const
	{
		return m_payload.size();
	}

public:

	bool ReadPayloadFromRequestHandle(HTTPRequestHandle handle)
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

};

class CUtilHTTPResponse : public IUtilHTTPResponse
{
private:
	bool m_bResponseCompleted{};
	bool m_bResponseError{};
	bool m_bIsStreamPayload{};
	int m_iResponseStatusCode{};
	CUtilHTTPPayload* m_pResponsePayload{};

	HTTPRequestHandle m_RequestHandle{ INVALID_HTTPREQUEST_HANDLE  };

public:

	void SetStreamPayload(bool bIsStreamPayload)
	{
		m_bIsStreamPayload = bIsStreamPayload;
	}

	bool OnSteamHTTPDataReceived(HTTPRequestDataReceived_t* pResult, bool bHasError, std::string &buf)
	{
		m_RequestHandle = pResult->m_hRequest;

		if (m_bIsStreamPayload && !bHasError)
		{
			buf.resize(pResult->m_cBytesReceived, '\0');

			if (SteamHTTP()->GetHTTPStreamingResponseBodyData(pResult->m_hRequest, pResult->m_cOffset,  (uint8*)buf.data(), pResult->m_cBytesReceived))
			{
				return true;
			}
		}

		return false;
	}

	void OnSteamHTTPCompleted(HTTPRequestCompleted_t* pResult, bool bHasError)
	{
		m_RequestHandle = pResult->m_hRequest;

		m_bResponseCompleted = true;
		m_bResponseError = bHasError;
		m_iResponseStatusCode = (int)pResult->m_eStatusCode;

		if (pResult->m_bRequestSuccessful && !bHasError && !m_bIsStreamPayload)
		{
			m_pResponsePayload->ReadPayloadFromRequestHandle(pResult->m_hRequest);
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
	CCallResult<CUtilHTTPRequest, HTTPRequestHeadersReceived_t> m_HeaderReceivedCallResult{};
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

		if(hCookieHandle != INVALID_HTTPCOOKIE_HANDLE)
			SteamHTTP()->SetHTTPRequestCookieContainer(m_RequestHandle, hCookieHandle);

		SetField("Host", field_host.c_str());
		SetField("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/84.0.4147.105 Safari/537.36");
	}

	~CUtilHTTPRequest()
	{
		if (m_HeaderReceivedCallResult.IsActive())
		{
			m_HeaderReceivedCallResult.Cancel();
		}
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

	virtual void OnSteamHTTPHeaderReceived(HTTPRequestHeadersReceived_t* pResult, bool bHasError)
	{
		OnRespondStart();
	}

	virtual void OnSteamHTTPCompleted(HTTPRequestCompleted_t* pResult, bool bHasError)
	{
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
			m_HeaderReceivedCallResult.Set(SteamApiCall, this, &CUtilHTTPRequest::OnSteamHTTPHeaderReceived);
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

	void SetPostBody(const char* contentType, const char* payload, size_t payloadSize) override
	{
		if (!contentType)
			contentType = "application/octet-stream";

		SteamHTTP()->SetHTTPRequestRawPostBody(m_RequestHandle, contentType, (uint8_t *)payload, payloadSize);
	}

	void SetField(const char* field, const char* value) override
	{
		SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, field, value);
	}

	void SetRequireCertVerification(bool b) override
	{
		SteamHTTP()->SetHTTPRequestRequiresVerifiedCertificate(m_RequestHandle, b);
	}

	void SetFollowLocation(bool b) override
	{
		//not supported
	}
};

class CUtilHTTPSyncRequest : public CUtilHTTPRequest
{
private:
	std::mutex m_mutex;
	std::condition_variable m_cv;
	bool m_isComplete{ false };

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
	}

	bool IsAsync() const override
	{
		return false;
	}

	bool IsStream() const override
	{
		return false;
	}

	void WaitForComplete() override
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_cv.wait(lock, [this]() { return m_isComplete; });
	}

	bool WaitForCompleteTimeout(int timeout_ms) override
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_cv.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]() { return m_isComplete; });
	}

	IUtilHTTPResponse* GetResponse() override
	{
		return m_pResponse;
	}

	void OnSteamHTTPCompleted(HTTPRequestCompleted_t* pResult, bool bHasError) override
	{
		CUtilHTTPRequest::OnSteamHTTPCompleted(pResult, bHasError);


		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_isComplete = true;
		}
		m_cv.notify_one();
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

	void WaitForComplete() override
	{

	}

	bool WaitForCompleteTimeout(int timeout_ms) override
	{
		return false;
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
	CCallResult<CUtilHTTPAsyncStreamRequest, HTTPRequestHeadersReceived_t> m_HeaderReceivedCallResult{};
	CCallResult<CUtilHTTPAsyncStreamRequest, HTTPRequestDataReceived_t> m_DataReceivedCallResult{};

	//IUtilHTTPCallbacks* m_StreamCallbacks{};

public:
	CUtilHTTPAsyncStreamRequest(
		const UtilHTTPMethod method,
		const std::string& host,
		unsigned short port,
		bool secure,
		const std::string& target,
		IUtilHTTPCallbacks* StreamCallbacks,
		HTTPCookieContainerHandle hCookieHandle) :
		CUtilHTTPAsyncRequest(method, host, port, secure, target, nullptr, hCookieHandle)//, m_StreamCallbacks(StreamCallbacks)
	{
		m_pResponse->SetStreamPayload(true);
	}

	~CUtilHTTPAsyncStreamRequest()
	{
		if (m_StreamCompleteCallResult.IsActive())
		{
			m_StreamCompleteCallResult.Cancel();
		}
		if (m_HeaderReceivedCallResult.IsActive())
		{
			m_HeaderReceivedCallResult.Cancel();
		}
		if (m_DataReceivedCallResult.IsActive())
		{
			m_DataReceivedCallResult.Cancel();
		}
	}

	bool IsStream() const override
	{
		return true;
	}

	void OnSteamHTTPHeaderReceived(HTTPRequestHeadersReceived_t* pResult, bool bHasError) override
	{
		OnRespondStart();
	}

	void OnSteamHTTPDataReceived(HTTPRequestDataReceived_t* pResult, bool bHasError)
	{
		std::string buf;
		m_pResponse->OnSteamHTTPDataReceived(pResult, bHasError, buf);

		if (m_Callbacks)
		{
			m_Callbacks->OnReceiveData(
				this,
				m_pResponse, 
				buf.data(),
				buf.size()
			);
		}
	}

	void OnSteamHTTPCompleted(HTTPRequestCompleted_t* pResult, bool bHasError) override
	{
		m_bRequestSuccessful = pResult->m_bRequestSuccessful;

		m_pResponse->OnSteamHTTPCompleted(pResult, bHasError);

		if (m_Callbacks)
		{
			m_Callbacks->OnResponseComplete(this, m_pResponse);
		}

		OnRespondFinish();
	}

	void OnRespondStart() override
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

	void OnRespondFinish() override
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

	void Send() override
	{
		SteamAPICall_t SteamApiCall;
		if (SteamHTTP()->SendHTTPRequestAndStreamResponse(m_RequestHandle, &SteamApiCall))
		{
			m_StreamCompleteCallResult.Set(SteamApiCall, this, &CUtilHTTPAsyncStreamRequest::OnSteamHTTPCompleted);
			m_HeaderReceivedCallResult.Set(SteamApiCall, this, &CUtilHTTPAsyncStreamRequest::OnSteamHTTPHeaderReceived);
			m_DataReceivedCallResult.Set(SteamApiCall, this, &CUtilHTTPAsyncStreamRequest::OnSteamHTTPDataReceived);
		}

		m_bRequesting = true;

		if (m_Callbacks)
		{
			m_Callbacks->OnUpdateState(UtilHTTPRequestState::Requesting);
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

					try {
						size_t pos;
						int port = std::stoi(port_str, &pos);
						if (pos != port_str.size() || port < 0 || port > 65535) {
							return nullptr;
						}
						port_us = static_cast<unsigned short>(port);
					}
					catch (const std::invalid_argument&) {
						return nullptr;
					}
					catch (const std::out_of_range&) {
						return nullptr;
					}
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


	IUtilHTTPRequest* CreateAsyncStreamRequestEx(const char* host, unsigned short port_us, const char* target, bool secure, const UtilHTTPMethod method, IUtilHTTPCallbacks* callback)
	{
		return new CUtilHTTPAsyncStreamRequest(method, host, port_us, secure, target, callback, m_CookieHandle);
	}

	IUtilHTTPRequest* CreateAsyncStreamRequest(const char* url, const UtilHTTPMethod method, IUtilHTTPCallbacks* callbacks) override
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