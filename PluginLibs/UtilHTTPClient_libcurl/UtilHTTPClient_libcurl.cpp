#include <Windows.h>
#include <format>
#include <regex>
#include <mutex>
#include <sstream>
#include <unordered_map>

#include <IUtilHTTPClient.h>

#include <ScopeExit/ScopeExit.h>

#include <curl/curl.h>

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
	std::stringstream m_stream;

public:

	const char* GetBytes() const override
	{
		return m_payload.data();
	}

	size_t GetLength() const override
	{
		return m_payload.size();
	}

public:

	void Reset()
	{
		m_stream.clear();
	}

	void Write(const void* data, size_t size)
	{
		m_stream.write((const char*)data, size);
	}

	void Finalize()
	{
		m_payload = m_stream.str();
		m_stream.clear();
	}
};

class CUtilHTTPResponse : public IUtilHTTPResponse
{
private:
	bool m_bResponseCompleted{};
	bool m_bResponseError{};
	bool m_bIsStream{};
	int m_iResponseStatusCode{};
	CUtilHTTPPayload* m_pResponsePayload{};
	CUtilHTTPPayload* m_pResponseHeaderPayload{};

	std::unordered_map<std::string, std::string> m_headers;

	CURL* m_CurlEasyHandle{};

public:
	CUtilHTTPResponse(CURL* CurlEasyHandle) :
		m_pResponsePayload(new CUtilHTTPPayload()), 
		m_pResponseHeaderPayload(new CUtilHTTPPayload()),
		m_CurlEasyHandle(CurlEasyHandle)
	{

	}

	~CUtilHTTPResponse()
	{
		if (m_pResponsePayload)
		{
			delete m_pResponsePayload;
			m_pResponsePayload = nullptr;
		}
		if (m_pResponseHeaderPayload)
		{
			delete m_pResponseHeaderPayload;
			m_pResponseHeaderPayload = nullptr;
		}
	}

	bool GetHeaderSize(const char* name, size_t* buflen) override
	{
		auto it = m_headers.find(name);
		if (it != m_headers.end()) {
			*buflen = it->second.length() + 1;
			return true;
		}
		return false;
	}

	bool GetHeader(const char* name, char* buf, size_t buflen) override
	{
		auto it = m_headers.find(name);
		if (it != m_headers.end()) {
			strncpy(buf, it->second.c_str(), buflen);
			return true;
		}
		return false;
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
public:

	void FinalizeHeaders()
	{
		m_pResponseHeaderPayload->Finalize();

		const auto& headersSv = std::string_view(m_pResponseHeaderPayload->GetBytes(), m_pResponseHeaderPayload->GetLength());

		size_t start = 0;
		size_t end = 0;

		// Iterate through the string_view to parse headers
		while ((end = headersSv.find("\r\n", start)) != std::string_view::npos) {
			auto line = headersSv.substr(start, end - start);
			if (!line.empty()) {
				// Find the position of ':'
				size_t delimiter_pos = line.find(':');
				if (delimiter_pos != std::string_view::npos) {
					// Extract key and value
					auto key = line.substr(0, delimiter_pos);
					auto value = line.substr(delimiter_pos + 1);

					// Trim spaces
					key = key.substr(0, key.find_last_not_of(" \t") + 1);
					value = value.substr(value.find_first_not_of(" \t"));
					value = value.substr(0, value.find_last_not_of(" \t") + 1);

					// Store in map
					m_headers[std::string(key)] = std::string(value);
				}
			}
			start = end + 2; // Move past "\r\n"
		}
	}

	void FinalizePayload()
	{
		m_pResponsePayload->Finalize();

		auto code = curl_easy_getinfo(m_CurlEasyHandle, CURLINFO_RESPONSE_CODE, &m_iResponseStatusCode);

		if (code != CURLE_OK || m_iResponseStatusCode >= 400 || !m_iResponseStatusCode)
		{
			m_bResponseError = true;
		}

		m_bResponseCompleted = true;
	}

	void WriteHeader(const void* data, size_t size)
	{
		m_pResponseHeaderPayload->Write(data, size);
	}

	void WritePayload(const void* data, size_t size)
	{
		m_pResponsePayload->Write(data, size);
	}
};

static size_t WritePayloadStreamCallback(void* contents, size_t size, size_t nmemb, void* userp);

static size_t WritePayloadCallback(void* contents, size_t size, size_t nmemb, void* userp);

static size_t WriteHeaderCallback(void* contents, size_t size, size_t nmemb, void* userp);

class CUtilHTTPRequest : public IUtilHTTPRequest
{
protected:
	CURLM* m_CurlMultiHandle{ nullptr };
	CURL* m_CurlEasyHandle{ nullptr };
	CURLSH* m_CurlCookieHandle{ nullptr };
	struct curl_slist* m_CurlHeaders{ nullptr };

	CUtilHTTPResponse* m_pResponse{ nullptr };
	IUtilHTTPCallbacks* m_Callbacks{ nullptr };
	bool m_bRequesting{ false };
	bool m_bResponding{ false };
	bool m_bFinished{ false };
	bool m_bRequestSuccessful{ false };//TODO: we need to set it to true when request become successful
	bool m_bAutoDestroyOnFinish{ false };
	UtilHTTPRequestId_t m_RequestId{ UTILHTTP_REQUEST_INVALID_ID };

public:

	CUtilHTTPRequest(
		const UtilHTTPMethod method,
		const std::string& host,
		unsigned short port,
		bool secure,
		const std::string& target,
		IUtilHTTPCallbacks* callbacks,
		CURLM*CurlMultiHandle,
		CURLSH* CurlCookieHandle) :
		m_Callbacks(callbacks), 
		m_CurlMultiHandle(CurlMultiHandle),
		m_CurlCookieHandle(CurlCookieHandle)
	{
		m_CurlEasyHandle = curl_easy_init();

		m_pResponse = new CUtilHTTPResponse(m_CurlEasyHandle);

		std::string url = std::format("{}://{}:{}{}",
			secure ? "https" : "http",
			host,
			port,
			target);

		curl_easy_setopt(m_CurlEasyHandle, CURLOPT_PRIVATE, this);
		curl_easy_setopt(m_CurlEasyHandle, CURLOPT_URL, url.c_str());
		curl_easy_setopt(m_CurlEasyHandle, CURLOPT_WRITEFUNCTION, WritePayloadCallback);
		curl_easy_setopt(m_CurlEasyHandle, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(m_CurlEasyHandle, CURLOPT_HEADERFUNCTION, WriteHeaderCallback);
		curl_easy_setopt(m_CurlEasyHandle, CURLOPT_HEADERDATA, this);
		curl_easy_setopt(m_CurlEasyHandle, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(m_CurlEasyHandle, CURLOPT_CONNECTTIMEOUT_MS, 60000);
		curl_easy_setopt(m_CurlEasyHandle, CURLOPT_TIMEOUT_MS, 60000);
		curl_easy_setopt(m_CurlEasyHandle, CURLOPT_COOKIEFILE, "");
		if(m_CurlCookieHandle)
			curl_easy_setopt(m_CurlEasyHandle, CURLOPT_SHARE, m_CurlCookieHandle);

		// Set method
		switch (method) {
		case UtilHTTPMethod::Get:
			curl_easy_setopt(m_CurlEasyHandle, CURLOPT_HTTPGET, 1L);
			break;
		case UtilHTTPMethod::Post:
			curl_easy_setopt(m_CurlEasyHandle, CURLOPT_POST, 1L);
			break;
		case UtilHTTPMethod::Put:
			curl_easy_setopt(m_CurlEasyHandle, CURLOPT_PUT, 1L);
			break;
		case UtilHTTPMethod::Delete:
			curl_easy_setopt(m_CurlEasyHandle, CURLOPT_CUSTOMREQUEST, "DELETE");
			break;
		case UtilHTTPMethod::Head:
			curl_easy_setopt(m_CurlEasyHandle, CURLOPT_NOBODY, 1L);
			break;
		}

		std::string field_host = host;

		if (secure && port != 443)
		{
			field_host = std::format("{0}:{1}", host, port);
		}
		else if (!secure && port != 80)
		{
			field_host = std::format("{0}:{1}", host, port);
		}

		SetField("Host", field_host.c_str());
		SetField("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/84.0.4147.105 Safari/537.36");

	}

	~CUtilHTTPRequest()
	{
		if (m_CurlHeaders) {
			curl_slist_free_all(m_CurlHeaders);
			m_CurlHeaders = nullptr;
		}
		if (m_CurlMultiHandle && m_CurlEasyHandle) {
			curl_multi_remove_handle(m_CurlMultiHandle, m_CurlEasyHandle);
		}
		if (m_CurlEasyHandle) {
			curl_easy_cleanup(m_CurlEasyHandle);
			m_CurlEasyHandle = nullptr;
		}
		if (m_pResponse) {
			delete m_pResponse;
			m_pResponse = nullptr;
		}
		if (m_Callbacks) {
			m_Callbacks->Destroy();
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

	virtual void OnHTTPComplete()
	{
		m_pResponse->FinalizeHeaders();
		m_pResponse->FinalizePayload();

		int httpStatusCode = 0;
		auto code = curl_easy_getinfo(m_CurlEasyHandle, CURLINFO_RESPONSE_CODE, &httpStatusCode);

		if (code != CURLE_OK || httpStatusCode >= 400 || !httpStatusCode)
		{

		}
		else
		{
			m_bRequestSuccessful = true;
		}

		if (m_Callbacks)
		{
			m_Callbacks->OnResponseComplete(this, m_pResponse);
		}

		OnRespondFinish();
	}

	void WriteHeader(const void* data, size_t size)
	{
		m_pResponse->WriteHeader(data, size);
	}

	void WritePayload(const void* data, size_t size)
	{
		m_pResponse->WritePayload(data, size);
	}

	void FinalizeHeaders()
	{
		m_pResponse->FinalizeHeaders();
	}
public:

	void Destroy() override
	{
		delete this;
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
		curl_easy_setopt(m_CurlEasyHandle, CURLOPT_TIMEOUT_MS, secs * 1000);
		curl_easy_setopt(m_CurlEasyHandle, CURLOPT_CONNECTTIMEOUT_MS, secs * 1000);
	}

	void SetPostBody(const char* contentType, const char* payload, size_t payloadSize) override
	{
		curl_easy_setopt(m_CurlEasyHandle, CURLOPT_POSTFIELDSIZE, payloadSize);
		curl_easy_setopt(m_CurlEasyHandle, CURLOPT_COPYPOSTFIELDS, payload);

		if(contentType && contentType[0])
			SetField("Content-Type", contentType);
	}

	void SetField(const char* field, const char* value) override
	{
		std::string header = std::format("{}: {}", field, value);
		m_CurlHeaders = curl_slist_append(m_CurlHeaders, header.c_str());
		curl_easy_setopt(m_CurlEasyHandle, CURLOPT_HTTPHEADER, m_CurlHeaders);
	}

	void Send() override
	{
		m_bRequesting = true;

		if (m_Callbacks) {
			m_Callbacks->OnUpdateState(UtilHTTPRequestState::Requesting);
		}

		curl_multi_add_handle(m_CurlMultiHandle, m_CurlEasyHandle);
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
		CURLM* CurlMultiHandle,
		CURLSH* CurlCookieHandle
		) :
		CUtilHTTPRequest(method, host, port, secure, target, callbacks, CurlMultiHandle, CurlCookieHandle)
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

	void OnRespondFinish() override
	{
		CUtilHTTPRequest::OnRespondFinish();

		if (m_hResponseEvent)
		{
			SetEvent(m_hResponseEvent);
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
};

class CUtilHTTPAsyncRequest : public CUtilHTTPRequest
{
protected:

public:
	CUtilHTTPAsyncRequest(
		const UtilHTTPMethod method,
		const std::string& host,
		unsigned short port,
		bool secure,
		const std::string& target,
		IUtilHTTPCallbacks* callbacks,
		CURLM* CurlMultiHandle,
		CURLSH* CurlCookieHandle)
		:
		CUtilHTTPRequest(method, host, port, secure, target, callbacks, CurlMultiHandle, CurlCookieHandle)
	{
		
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
	IUtilHTTPStreamCallbacks* m_StreamCallbacks{};

public:
	CUtilHTTPAsyncStreamRequest(
		const UtilHTTPMethod method,
		const std::string& host,
		unsigned short port,
		bool secure,
		const std::string& target,
		IUtilHTTPStreamCallbacks* StreamCallbacks,
		CURLM* CurlMultiHandle,
		CURLSH* CurlCookieHandle) :
		CUtilHTTPAsyncRequest(method, host, port, secure, target, nullptr, CurlMultiHandle, CurlCookieHandle),
		m_StreamCallbacks(StreamCallbacks)
	{

		curl_easy_setopt(m_CurlEasyHandle, CURLOPT_WRITEFUNCTION, WritePayloadStreamCallback);
		curl_easy_setopt(m_CurlEasyHandle, CURLOPT_WRITEDATA, this);

	}

	~CUtilHTTPAsyncStreamRequest()
	{
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

	void OnHTTPComplete() override
	{
		m_pResponse->FinalizeHeaders();
		m_pResponse->FinalizePayload();

		int httpStatusCode = 0;
		auto code = curl_easy_getinfo(m_CurlEasyHandle, CURLINFO_RESPONSE_CODE, &httpStatusCode);

		if (code != CURLE_OK || httpStatusCode >= 400 || !httpStatusCode)
		{

		}
		else
		{
			m_bRequestSuccessful = true;
		}

		if (m_StreamCallbacks)
		{
			m_StreamCallbacks->OnResponseComplete(this, m_pResponse);
		}

		OnRespondFinish();
	}

public:

	void WritePayloadStream(const void* data, size_t size)
	{
		if (m_StreamCallbacks)
		{
			m_StreamCallbacks->OnReceiveData(this, m_pResponse, data, size);
		}
	}
};

static size_t WritePayloadStreamCallback(void* contents, size_t size, size_t nmemb, void* userp) {

	auto pRequest = static_cast<CUtilHTTPAsyncStreamRequest*>(userp);

	size_t realsize = size * nmemb;

	pRequest->WritePayloadStream(contents, realsize);

	return realsize;
}

static size_t WritePayloadCallback(void* contents, size_t size, size_t nmemb, void* userp) {

	auto pRequest = static_cast<CUtilHTTPRequest*>(userp);

	size_t realsize = size * nmemb;

	pRequest->WritePayload(contents, realsize);

	return realsize;
}

static size_t WriteHeaderCallback(void* contents, size_t size, size_t nmemb, void* userp) {

	auto pRequest = static_cast<CUtilHTTPRequest*>(userp);

	size_t realsize = size * nmemb;

	pRequest->OnRespondStart();

	pRequest->WriteHeader(contents, realsize);

	return realsize;
}

class CUtilHTTPClient : public IUtilHTTPClient
{
private:
	std::mutex m_RequestHandleLock;
	UtilHTTPRequestId_t m_RequestUsedId{ UTILHTTP_REQUEST_START_ID };
	std::unordered_map<UtilHTTPRequestId_t, IUtilHTTPRequest*> m_RequestPool;
	CURLM* m_CurlMultiHandle{ };
	CURLSH* m_CurlCookieHandle{ };

public:

	CUtilHTTPClient()
	{
		curl_global_init(CURL_GLOBAL_ALL);

		curl_global_sslset(CURLSSLBACKEND_SCHANNEL, NULL, NULL);
	}

	~CUtilHTTPClient()
	{
		curl_global_cleanup();
	}

	void Destroy() override
	{
		delete this;
	}

	void Init(const CUtilHTTPClientCreationContext* context) override
	{
		if (context->m_bUseCookieContainer)
		{
			m_CurlCookieHandle = curl_share_init();

			// Set share options
			curl_share_setopt(m_CurlCookieHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
		}

		m_CurlMultiHandle = curl_multi_init();
	}

	void Shutdown() override
	{
		std::lock_guard<std::mutex> lock(m_RequestHandleLock);

		for (auto itor = m_RequestPool.begin(); itor != m_RequestPool.end(); itor ++)
		{
			auto pRequest = (*itor).second;

			pRequest->Destroy();
		}

		m_RequestPool.clear();

		if (m_CurlMultiHandle)
		{
			curl_multi_cleanup(m_CurlMultiHandle);
			m_CurlMultiHandle = nullptr;
		}

		if (m_CurlCookieHandle)
		{
			curl_share_cleanup(m_CurlCookieHandle);
			m_CurlCookieHandle = nullptr;
		}
	}

	void PerformMultiHandle()
	{
		int runningHandles = 0;
		auto code = curl_multi_perform(m_CurlMultiHandle, &runningHandles);
		if (code == CURLM_OK || code == CURLM_CALL_MULTI_PERFORM)
		{
			int msg_queued = 0;
			CURLMsg* msg = NULL;
			do
			{
				msg = curl_multi_info_read(m_CurlMultiHandle, &msg_queued);
				if (msg && (msg->msg == CURLMSG_DONE))
				{
					auto eh = msg->easy_handle;

					CUtilHTTPRequest* pRequest = NULL;

					curl_easy_getinfo(eh, CURLINFO_PRIVATE, &pRequest);

					if (pRequest)
					{
						pRequest->OnHTTPComplete();
					}
				}
			} while (msg);
		}
	}

	void RunFrame() override
	{
		PerformMultiHandle();

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
		return new CUtilHTTPSyncRequest(method, host, port_us, secure, target, callback, m_CurlMultiHandle, m_CurlCookieHandle);
	}

	IUtilHTTPRequest* CreateAsyncRequestEx(const char * host, unsigned short port_us, const char* target, bool secure, const UtilHTTPMethod method, IUtilHTTPCallbacks* callback)
	{
		return new CUtilHTTPAsyncRequest(method, host, port_us, secure, target, callback, m_CurlMultiHandle, m_CurlCookieHandle);
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
		return new CUtilHTTPAsyncStreamRequest(method, host, port_us, secure, target, callback, m_CurlMultiHandle, m_CurlCookieHandle);
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
		//TODO: ....

		return false;
	}

};

EXPOSE_INTERFACE(CUtilHTTPClient, IUtilHTTPClient, UTIL_HTTPCLIENT_LIBCURL_INTERFACE_VERSION);