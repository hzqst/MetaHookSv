---
title: UtilHTTPClient_Steam
type: note
permalink: metahooksv/util-httpclient-steam
---

# UtilHTTPClient_Steam Source Analysis (Actual Directory: PluginLibs/UtilHTTPClient_SteamAPI)

## Overview
`PluginLibs/UtilHTTPClient_SteamAPI` is an HTTP client implementation library based on Steamworks `ISteamHTTP` (`steam_api.h`). It provides unified `IUtilHTTPClient`/`IUtilHTTPRequest`/`IUtilHTTPResponse` interfaces to other modules in the project (see `include/Interface/IUtilHTTPClient.h`), and exports interface versions through `EXPOSE_INTERFACE`/`EXPOSE_SINGLE_INTERFACE`:
- `UTIL_HTTPCLIENT_STEAMAPI_INTERFACE_VERSION` = `UtilHTTPClient_SteamAPI_007`
- `UTIL_HTTPCLIENT_FACTORY_STEAMAPI_INTERFACE_VERSION` = `UtilHTTPClientFactory_SteamAPI_007`

The core implementation is concentrated in:
- `PluginLibs/UtilHTTPClient_SteamAPI/UtilHTTPClient_SteamAPI.cpp`

## Responsibilities
- Provides HTTP request creation and sending: synchronous requests (which can block while waiting), asynchronous requests (callback-driven), and asynchronous streaming requests (received by chunk).
- Abstracts the Steam HTTP API: method mapping, header configuration, POST body configuration, certificate-validation switching, and cookie containers.
- Provides a "request pool + per-frame cleanup" mechanism (`RunFrame()`) for asynchronous requests, so callers do not need to manage lifetimes themselves.

## Architecture
### Interface Layer (`include/Interface/IUtilHTTPClient.h`)
- `IUtilHTTPClient`
  - `Init/Shutdown/RunFrame`
  - `CreateSyncRequest/CreateAsyncRequest/CreateAsyncStreamRequest`
  - `AddToRequestPool/GetRequestById/DestroyRequestById`
  - `SetCookie`
- `IUtilHTTPRequest`
  - `Send`, header/body/timeout, and other configuration
  - State queries (`UtilHTTPRequestState`)
  - Synchronous-request `WaitForComplete/WaitForCompleteTimeout/GetResponse`
- `IUtilHTTPResponse`
  - Status code, headers, payload
- `IUtilHTTPCallbacks`
  - `OnUpdateState/OnReceiveData/OnResponseComplete` + `Destroy()`
- `IURLParsedResult`
  - URL parsing-result carrier (scheme/host/port/target/secure)

### Implementation Layer (`PluginLibs/UtilHTTPClient_SteamAPI/UtilHTTPClient_SteamAPI.cpp`)
- `UTIL_ConvertUtilHTTPMethodToSteamHTTPMethod`: `UtilHTTPMethod -> EHTTPMethod` mapping.
- `ParseUrlInternal` + `CURLParsedResult`: URL parsing (returns `IURLParsedResult*`).
- `CUtilHTTPPayload`: Encapsulates the response body (reads the complete body for non-streaming requests).
- `CUtilHTTPResponse`: Encapsulates the response status code, headers (with cache), and payload; interfaces with Steam callbacks.
- `CUtilHTTPRequest`: Request base class (implements `IUtilHTTPRequest`), holding `HTTPRequestHandle` and Steam `CCallResult`:
  - `SendHTTPRequest` triggers a request
  - `OnSteamHTTPHeaderReceived/OnSteamHTTPCompleted` drive state transitions and callbacks
  - Its destructor cancels `CallResult`, calls `ReleaseHTTPRequest`, and invokes `m_Callbacks->Destroy()`
- `CUtilHTTPSyncRequest`: Synchronous request implementation
  - Uses `condition_variable` to wait for the completion flag set by `OnSteamHTTPCompleted`
  - `GetResponse()` returns `m_pResponse`
- `CUtilHTTPAsyncRequest`: Asynchronous request implementation
  - Defaults to `m_bAutoDestroyOnFinish = true`
  - `WaitForComplete/WaitForCompleteTimeout` are stubs, and `GetResponse()` directly returns `nullptr`
- `CUtilHTTPAsyncStreamRequest`: Asynchronous streaming request implementation
  - Uses `SendHTTPRequestAndStreamResponse`
  - Retrieves chunk data through `HTTPRequestDataReceived_t` and attempts to invoke `OnReceiveData`
- `CUtilHTTPClient`: HTTP Client instance
  - Optionally creates a cookie container (`CreateCookieContainer`)
  - Request pool: `unordered_map<UtilHTTPRequestId_t, IUtilHTTPRequest*> m_RequestPool`
  - `RunFrame()` cleanup: if a request is `IsFinished && IsAutoDestroyOnFinish`, it calls `Destroy + erase`
- `CUtilHTTPClientFactory`: Factory
  - `CreateUtilHTTPClient()` returns `new CUtilHTTPClient`
  - `ParseUrl()` directly reuses `ParseUrlInternal`

## Core Implementation and Workflow
### 1) Client Initialization/Loop
1. Create `IUtilHTTPClient` through the factory-exported interface (version `*_007`).
2. `IUtilHTTPClient::Init(context)`: If `context->m_bUseCookieContainer` is set, create an `HTTPCookieContainerHandle`.
3. Call `IUtilHTTPClient::RunFrame()` every frame: cleans up requests managed by the request pool that are automatically destroyed after completion.

Note: Steam's `CCallResult` callbacks depend on externally running the Steamworks callback pump regularly (typically `SteamAPI_RunCallbacks()`); otherwise, synchronous waits may deadlock.

### 2) Synchronous Requests (Sync)
- Call: `CreateSyncRequest(url, method, callbacks)`
  - Internally, `ParseUrlInternal(url)` -> `CreateSyncRequestEx(host, port, target, secure, ...)` -> `new CUtilHTTPSyncRequest`.
- Configuration: `SetTimeout/SetField/SetPostBody/SetRequireCertVerification`.
- Send: `Send()`
  - `SteamHTTP()->SendHTTPRequest` + bind `m_CompleteCallResult/m_HeaderReceivedCallResult`
  - Immediately invokes `callbacks->OnUpdateState(..., Requesting)`
- Wait: `WaitForComplete()` or `WaitForCompleteTimeout()`
  - `CUtilHTTPSyncRequest::OnSteamHTTPCompleted` sets the completion flag and wakes the waiter.
- Retrieve the result: `GetResponse()` -> `IUtilHTTPResponse`
  - For non-streaming requests, the body is read into `CUtilHTTPPayload` upon completion.

### 3) Asynchronous Requests (Async)
- Call: `CreateAsyncRequest(url, method, callbacks)` -> `new CUtilHTTPAsyncRequest` (automatically destroyed by default).
- Send: `Send()` is the same as for synchronous requests, but does not wait.
- Lifetime:
  - The caller either: lets the client manage the request with `client->AddToRequestPool(request)` (recommended) and queries it by request ID;
  - Or: retains it and calls `Destroy()` after completion.

### 4) Asynchronous Streaming Requests (AsyncStream)
- Sending uses `SteamHTTP()->SendHTTPRequestAndStreamResponse` and listens for:
  - `HTTPRequestHeadersReceived_t`: enters Responding
  - `HTTPRequestDataReceived_t`: reads a chunk and invokes `IUtilHTTPCallbacks::OnReceiveData`
  - `HTTPRequestCompleted_t`: completion callback

## Dependencies
- Steamworks SDK:
  - Include: `include/SteamSDK` (see `PluginLibs/UtilHTTPClient_SteamAPI/UtilHTTPClient_SteamAPI.vcxproj`)
  - Link: `steam_api.lib`
  - Code dependencies: `steam_api.h`, `SteamHTTP()`, `CCallResult<...>`
- C++20: `std::format` (`<format>`) and `std::regex` (`<regex>`); the project explicitly sets `LanguageStandard=stdcpp20`.
- ScopeExit: `<ScopeExit/ScopeExit.h>` (the project provides the include path through `$(ScopeExitIncludeDirectory)`).
- Windows: `<Windows.h>`
- Project interface: `include/Interface/IUtilHTTPClient.h`

## Considerations / Risk Points
3. `CUtilHTTPAsyncRequest`'s `WaitForComplete/WaitForCompleteTimeout/GetResponse` are stubs/return `nullptr`: asynchronous requests must consume results through callbacks. This is expected behavior.
5. Callback lifetime convention: the `CUtilHTTPRequest` destructor calls `m_Callbacks->Destroy()`. Therefore, callbacks must be objects "owned by the request" (typically heap-allocated), and `Destroy()` must delete itself; otherwise, double-free or freeing a stack object may occur.
6. `SetFollowLocation` is not implemented in the SteamAPI backend (directly `//not supported`).
7. The `m_ResponseErrorMessage` returned by `CUtilHTTPResponse::GetResponseErrorMessage()` is never populated, so error information may always be empty; you can only rely on `IsResponseError/IsRequestSuccessful/StatusCode`. This is because SteamAPI does not provide error information.
8. Synchronous waits depend on the Steam callback pump: if the external caller does not continuously run `SteamAPI_RunCallbacks()`, `WaitForComplete()` may block permanently. This is expected behavior.
10. The default `User-Agent` is hard-coded as a Chrome UA; some server policies may depend on the UA (it can be overridden with `SetField`).
