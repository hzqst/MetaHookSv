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

using fnSteamHTTPCompleteCallback = std::function<void(CUtilHTTPRequest* pRequestInstance, HTTPRequestCompleted_t* pResult, bool bHasError) >;

class CUtilHTTPRequest : public IUtilHTTPRequest, public IUtilHTTPResponse
{
private:
	HTTPRequestHandle m_RequestHandle{};
	CCallResult<CUtilHTTPRequest, HTTPRequestCompleted_t> m_CallResult{};
	fnSteamHTTPCompleteCallback m_SteamHTTPCompleteCallback{};
	HANDLE m_hResponseEvent{};
	bool m_bRequestSuccessful{};
	bool m_bResponseCompleted{};
	bool m_bResponseError{};
	int m_iResponseStatusCode{};

public:
	CUtilHTTPRequest(
		const UtilHTTPMethod method,
		const std::string& host,
		unsigned short port,
		bool secure,
		const std::string& target)
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

		SetField(UtilHTTPField::host, field_host);
		SetField(UtilHTTPField::user_agent, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/84.0.4147.105 Safari/537.36");

		m_hResponseEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
	}

	~CUtilHTTPRequest()
	{
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
	}

	void OnSteamHTTPCompleted(HTTPRequestCompleted_t* pResult, bool bHasError)
	{
		m_bRequestSuccessful = pResult->m_bRequestSuccessful;
		m_bResponseCompleted = true;
		m_bResponseError = bHasError;
		m_iResponseStatusCode = (int)pResult->m_eStatusCode;
		
		if (m_SteamHTTPCompleteCallback)
		{
			m_SteamHTTPCompleteCallback(this, pResult, bHasError);
		}

		if (m_hResponseEvent)
		{
			SetEvent(m_hResponseEvent);
		}
	}

	void SendAsyncRequest()
	{
		SteamAPICall_t apiCall;
		bool bRet = SteamHTTP()->SendHTTPRequest(m_RequestHandle, &apiCall);

		if (bRet)
		{
			m_CallResult.Set(apiCall, this, &CUtilHTTPRequest::OnSteamHTTPCompleted);
		}
	}

	void WaitForResponse()
	{
		if (m_hResponseEvent)
		{
			WaitForSingleObject(m_hResponseEvent, INFINITE);
		}
	}

	void SetSteamHTTPCompleteCallback(const fnSteamHTTPCompleteCallback &callback)
	{
		m_SteamHTTPCompleteCallback = callback;
	}

	bool IsResponseError() const
	{
		return m_bResponseError;
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

	void SetTimeout(int secs)  override
	{
		SteamHTTP()->SetHTTPRequestNetworkActivityTimeout(m_RequestHandle, secs);
	}

	void SetBody(const std::string& payload) override
	{
		//You can change content-type later
		SteamHTTP()->SetHTTPRequestRawPostBody(m_RequestHandle, "application/octet-stream", (uint8_t *)payload.data(), payload.size());
	}

	void SetField(const std::string& field, const std::string& value) override
	{
		SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, field.c_str(), value.c_str());
	}

	void SetField(UtilHTTPField field, const std::string& value) override
	{
		switch (field)
		{
		case UtilHTTPField::user_agent:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "User-Agent", value.c_str());
			break;
		case UtilHTTPField::uri:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Uri", value.c_str());
			break;
		case UtilHTTPField::set_cookie:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Set-Cookie", value.c_str());
			break;
		case UtilHTTPField::referer:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Referer", value.c_str());
			break;
		case UtilHTTPField::path:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Path", value.c_str());
			break;
		case UtilHTTPField::origin:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Origin", value.c_str());
			break;
		case UtilHTTPField::location:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Location", value.c_str());
			break;
		case UtilHTTPField::keep_alive:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Keep-Alive", value.c_str());
			break;
		case UtilHTTPField::host:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Host", value.c_str());
			break;
		case UtilHTTPField::expires:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Expires", value.c_str());
			break;
		case UtilHTTPField::expiry_date:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Expiry-Date", value.c_str());
			break;
		case UtilHTTPField::encoding:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Encoding", value.c_str());
			break;
		case UtilHTTPField::cookie:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Cookie", value.c_str());
			break;
		case UtilHTTPField::content_type:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Content-Type", value.c_str());
			break;
		case UtilHTTPField::content_length:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Content-Length", value.c_str());
			break;
		case UtilHTTPField::content_encoding:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Content-Encoding", value.c_str());
			break;
		case UtilHTTPField::cache_control:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Cache-Control", value.c_str());
			break;
		case UtilHTTPField::body:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Cache-Body", value.c_str());
			break;
		case UtilHTTPField::authorization:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Authorization", value.c_str());
			break;
		case UtilHTTPField::accept_encoding:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Accept-Encoding", value.c_str());
			break;
		case UtilHTTPField::accept_charset:
			SteamHTTP()->SetHTTPRequestHeaderValue(m_RequestHandle, "Accept-Charset", value.c_str());
			break;
		default:
			break;
		}

	}

	int GetStatusCode() const override
	{
		return m_iResponseStatusCode;
	}

	bool GetBody(std::string& body) const override
	{
		uint32_t responseSize = 0;
		if (SteamHTTP()->GetHTTPResponseBodySize(m_RequestHandle, &responseSize))
		{
			body.resize(responseSize);
			if (SteamHTTP()->GetHTTPResponseBodyData(m_RequestHandle, (uint8 *)body.data(), body.size() ))
			{
				return true;
			}
		}

		return false;
	}
};

class CUtilHTTPClient : public IUtilHTTPClient
{
private:
	std::mutex m_RequestHandleLock;
	UtilHTTPRequestHandle_t m_UsedRequestHandle{ UTILHTTP_START_REQUESTHANDLE };
	std::unordered_map<UtilHTTPRequestHandle_t, CUtilHTTPRequest*> m_RequestPool;

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
			auto RequestInstance = itor->second;

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
			auto RequestInstance = itor->second;

			if (RequestInstance->IsResponseCompleted())
			{
				delete RequestInstance;
				itor = m_RequestPool.erase(itor);
				continue;
			}

			itor++;
		}
	}

	UtilHTTPRequestHandle_t AddRequestToPool(CUtilHTTPRequest *RequestInstance)
	{
		std::lock_guard<std::mutex> lock(m_RequestHandleLock);

		if (m_UsedRequestHandle == UTILHTTP_MAX_REQUESTHANDLE)
			m_UsedRequestHandle = UTILHTTP_START_REQUESTHANDLE;

		auto Handle = m_UsedRequestHandle;

		m_UsedRequestHandle++;

		m_RequestPool[Handle] = RequestInstance;

		return m_UsedRequestHandle;
	}

	bool ParseUrl(const std::string& url, CURLParsedResult& result) override
	{
		std::regex url_regex(
			R"((http|https)://([-A-Z0-9+&@#/%?=~_|!:,.;]*[-A-Z0-9+&@#/%=~_|]):?(\d+)?([-A-Z0-9+&@#/%?=~_|!:,.;]*)?)",
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

				result.scheme = scheme;
				result.host = host;
				result.port_str = port_str;
				result.target = target;
				result.port_us = port_us;
				result.secure = false;

				if (scheme == "https") {
					result.secure = true;
				}

				return true;
			}
		}

		return false;
	}

	bool SyncRequest(const std::string& host, unsigned short port_us, const std::string& target, const UtilHTTPMethod method,
		const fnHTTPRequestCallback& request_callback, const fnHTTPResponseCallback& response_callback, const fnHTTPErrorCallback& error_callback)
	{
		CUtilHTTPRequest RequestInstance(method, host, port_us, false, target);

		request_callback(&RequestInstance);

		RequestInstance.SendAsyncRequest();

		RequestInstance.WaitForResponse();

		if (!RequestInstance.IsRequestSuccessful() || RequestInstance.IsResponseError())
		{
			error_callback(std::error_code());
			return false;
		}

		response_callback(&RequestInstance);
		return true;
	}

	bool SyncRequestTLS(const std::string& host, unsigned short port_us, const std::string& target, const UtilHTTPMethod method,
		const fnHTTPRequestCallback& request_callback, const fnHTTPResponseCallback& response_callback, const fnHTTPErrorCallback& error_callback)
	{
		CUtilHTTPRequest RequestInstance(method, host, port_us, true, target);

		request_callback(&RequestInstance);

		RequestInstance.SendAsyncRequest();

		RequestInstance.WaitForResponse();

		if (!RequestInstance.IsRequestSuccessful() || RequestInstance.IsResponseError())
		{
			error_callback(std::error_code());
			return false;
		}

		response_callback(&RequestInstance);
		return true;
	}

	bool Get(const std::string& url, const fnHTTPRequestCallback& request_callback, const fnHTTPResponseCallback& response_callback, const fnHTTPErrorCallback& error_callback) override 
	{
		CURLParsedResult result;

		if (!ParseUrl(url, result))
		{
			return false;
		}

		if (result.secure)
		{
			return SyncRequestTLS(result.host, result.port_us, result.target, UtilHTTPMethod::Get, request_callback, response_callback, error_callback);
		}

		return SyncRequest(result.host, result.port_us, result.target, UtilHTTPMethod::Get, request_callback, response_callback, error_callback);
	}

	bool Post(const std::string& url, const std::string& payload, const fnHTTPRequestCallback& request_callback, const fnHTTPResponseCallback& response_callback, const fnHTTPErrorCallback& error_callback) override 
	{
		CURLParsedResult result;

		if (!ParseUrl(url, result))
		{
			return false;
		}

		if (result.secure)
		{
			return SyncRequestTLS(result.host, result.port_us, result.target, UtilHTTPMethod::Post, [&payload, &request_callback](IUtilHTTPRequest* RequestInstance) {

				RequestInstance->SetBody(payload);
				RequestInstance->SetField(UtilHTTPField::content_length, std::format("{0}", payload.size()));

				request_callback(RequestInstance);

			}, response_callback, error_callback);
		}

		return SyncRequest(result.host, result.port_us, result.target, UtilHTTPMethod::Post, [&payload, &request_callback](IUtilHTTPRequest* RequestInstance) {

			RequestInstance->SetBody(payload);
			RequestInstance->SetField(UtilHTTPField::content_length, std::format("{0}", payload.size()));

			request_callback(RequestInstance);

		}, response_callback, error_callback);
	}

	UtilHTTPRequestHandle_t AsyncRequest(const std::string& host, unsigned short port_us, const std::string& target, const UtilHTTPMethod method, const fnHTTPRequestCallback& request_callback, const fnHTTPResponseCallback& response_callback, const fnHTTPErrorCallback& error_callback)
	{
		auto RequestInstance = new CUtilHTTPRequest(method, host, port_us, false, target);
		
		auto RequestHandle = AddRequestToPool(RequestInstance);

		request_callback(RequestInstance);

		RequestInstance->SetSteamHTTPCompleteCallback([response_callback, error_callback](CUtilHTTPRequest* pRequestInstance, HTTPRequestCompleted_t* pResult, bool bHasError) {

			if (!pRequestInstance->IsRequestSuccessful() || pRequestInstance->IsResponseError())
			{
				error_callback(std::error_code());
				return;
			}

			response_callback(pRequestInstance);

		});

		RequestInstance->SendAsyncRequest();

		return RequestHandle;
	}

	UtilHTTPRequestHandle_t AsyncRequestTLS(const std::string& host, unsigned short port_us, const std::string& target, const UtilHTTPMethod method, const fnHTTPRequestCallback& request_callback, const fnHTTPResponseCallback& response_callback, const fnHTTPErrorCallback& error_callback)
	{
		auto RequestInstance = new CUtilHTTPRequest(method, host, port_us, true, target);

		auto RequestHandle = AddRequestToPool(RequestInstance);

		request_callback(RequestInstance);

		RequestInstance->SetSteamHTTPCompleteCallback([response_callback, error_callback](CUtilHTTPRequest* pRequestInstance, HTTPRequestCompleted_t* pResult, bool bHasError) {

			if (!pRequestInstance->IsRequestSuccessful() || pRequestInstance->IsResponseError())
			{
				error_callback(std::error_code());
				return;
			}

			response_callback(pRequestInstance);

		});

		RequestInstance->SendAsyncRequest();

		return RequestHandle;
	}

	UtilHTTPRequestHandle_t AsyncGet(const std::string& url, const fnHTTPRequestCallback& request_callback, const fnHTTPResponseCallback& response_callback, const fnHTTPErrorCallback& error_callback) override
	{
		CURLParsedResult result;

		if (!ParseUrl(url, result))
		{
			return UTILHTTP_INVALID_REQUESTHANDLE;
		}

		if (result.secure)
		{
			return AsyncRequestTLS(result.host, result.port_us, result.target, UtilHTTPMethod::Get, request_callback, response_callback, error_callback);
		}

		return AsyncRequestTLS(result.host, result.port_us, result.target, UtilHTTPMethod::Get, request_callback, response_callback, error_callback);
	}

	UtilHTTPRequestHandle_t AsyncPost(const std::string& url, const std::string& payload, const fnHTTPRequestCallback& request_callback, const fnHTTPResponseCallback& response_callback, const fnHTTPErrorCallback& error_callback) override
	{
		CURLParsedResult result;
		if (!ParseUrl(url, result))
		{
			return UTILHTTP_INVALID_REQUESTHANDLE;
		}

		if (result.secure)
		{
			return AsyncRequestTLS(result.host, result.port_us, result.target, UtilHTTPMethod::Post, [&payload, &request_callback](IUtilHTTPRequest* RequestInstance) {

				RequestInstance->SetBody(payload);
				RequestInstance->SetField(UtilHTTPField::content_length, std::format("{0}", payload.size()));

				request_callback(RequestInstance);

			}, response_callback, error_callback);
		}

		return AsyncRequest(result.host, result.port_us, result.target, UtilHTTPMethod::Post, [&payload, &request_callback](IUtilHTTPRequest* RequestInstance) {

			RequestInstance->SetBody(payload);
			RequestInstance->SetField(UtilHTTPField::content_length, std::format("{0}", payload.size()));

			request_callback(RequestInstance);

		}, response_callback, error_callback);
	}

};

EXPOSE_SINGLE_INTERFACE(CUtilHTTPClient, IUtilHTTPClient, UTIL_HTTPCLIENT_STEAMAPI_INTERFACE_VERSION);