---
title: UtilHTTPClient_libcurl
type: note
permalink: metahooksv/util-httpclient-libcurl
---

# UtilHTTPClient_libcurl (Source-Level Analysis)

## Overview
`PluginLibs/UtilHTTPClient_libcurl` is an HTTP client implementation based on **libcurl (multi/easy)**. As a MetaHookSv utility library, it exports the `IUtilHTTPClient` and `IUtilHTTPClientFactory` interfaces as a DLL (see `PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`).

This implementation is designed to:
- Drive network requests once per frame from the engine/plugin main loop (`IUtilHTTPClient::RunFrame()`).
- Support synchronous/asynchronous requests and streaming reception (chunk callbacks).
- Provide an optional Cookie container that is shared across requests.

## Responsibilities
- **`IUtilHTTPClient` implementation**: Creates and manages request objects, drives libcurl multi, and provides request-pool (`id -> request`) lifecycle management.
- **Request abstraction**: Maps the `IUtilHTTPRequest` API (`Send/SetTimeout/SetPostBody/SetField/...`) to libcurl easy options.
- **Response abstraction**: Collects headers/bodies into `IUtilHTTPResponse`/`IUtilHTTPPayload` and provides header queries.
- **Callback dispatch**: Uses `IUtilHTTPCallbacks` to deliver state changes, streamed data, and completion events.

## Architecture
Core types and relationships (all in `PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`):
- `CUtilHTTPClient` (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`):
  - Holds `CURLM* m_CurlMultiHandle` (multi) and an optional `CURLSH* m_CurlCookieHandle` (shared Cookie handle).
  - Maintains the request pool: `unordered_map<UtilHTTPRequestId_t, IUtilHTTPRequest*> m_RequestPool` + `m_RequestHandleLock`.
  - `RunFrame()` drives `curl_multi_perform()`, handles `CURLMSG_DONE`, and then cleans up auto-destroy requests.
- `CUtilHTTPRequest` (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`):
  - Base class: wraps a `CURL* m_CurlEasyHandle` and forwards libcurl callbacks (header/body) to `CUtilHTTPResponse`.
  - Holds `CUtilHTTPResponse* m_pResponse` and `IUtilHTTPCallbacks* m_Callbacks` (the destructor calls `m_Callbacks->Destroy()`).
- `CUtilHTTPSyncRequest` (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`):
  - Sets the completion state and calls `notify_one()` in `OnRespondFinish()`, providing the condition variable for `WaitForComplete()`.
- `CUtilHTTPAsyncRequest` (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`):
  - A purely asynchronous wrapper; `WaitForComplete/GetResponse` are empty implementations/return `nullptr`.
- `CUtilHTTPAsyncStreamRequest` (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`):
  - Overrides the easy write callback and directly calls `IUtilHTTPCallbacks::OnReceiveData()` when a chunk arrives.
- `CUtilHTTPResponse` (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`):
  - Maintains one `CUtilHTTPPayload` buffer each for headers and body; `FinalizeHeaders()` parses header lines into `m_headers`.
- `CUtilHTTPPayload` (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`):
  - Aggregates data with `stringstream` and stores it in `std::string m_payload` after `Finalize()`.
- `CURLParsedResult`/`ParseUrlInternal` (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`):
  - URL parsing result object (implements `IURLParsedResult`).

The interface is defined in `include/Interface/IUtilHTTPClient.h`.

## Core Implementation and Workflow
### 1) Module Exports and Creation
- Exports the client through `EXPOSE_INTERFACE(CUtilHTTPClient, IUtilHTTPClient, UTIL_HTTPCLIENT_LIBCURL_INTERFACE_VERSION)` (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`).
- Exports the factory through `EXPOSE_SINGLE_INTERFACE(CUtilHTTPClientFactory, IUtilHTTPClientFactory, UTIL_HTTPCLIENT_FACTORY_LIBCURL_INTERFACE_VERSION)` (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`).
- Version macros are defined in `include/Interface/IUtilHTTPClient.h` (such as `UTIL_HTTPCLIENT_LIBCURL_INTERFACE_VERSION`).

### 2) Initialization/Shutdown (multi + Cookie sharing)
- `CUtilHTTPClient::Init(context)`:
  - If `context->m_bUseCookieContainer` is true, calls `curl_share_init()` and `curl_share_setopt(..., CURL_LOCK_DATA_COOKIE)` (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`).
  - Creates `m_CurlMultiHandle = curl_multi_init()`.
- `CUtilHTTPClient::Shutdown()`:
  - Destroys every request in the request pool, then cleans up multi and share (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`).

### 3) Request Creation (easy handle configuration)
- When constructing `CUtilHTTPRequest`:
  - Calls `curl_easy_init()` and constructs `CUtilHTTPResponse(m_CurlEasyHandle)`.
  - Builds the URL: `std::format("{}://{}:{}{}", ...)`.
  - Binds callbacks: `CURLOPT_WRITEFUNCTION/WRITEDATA`, `CURLOPT_HEADERFUNCTION/HEADERDATA` (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`).
  - Sets default network parameters: `CONNECTTIMEOUT_MS/TIMEOUT_MS=60000`, `CURLOPT_ACCEPT_ENCODING=""` enables decompression, and `CURLOPT_COOKIEFILE=""` enables the Cookie engine.
  - If Cookie sharing is enabled: `CURLOPT_SHARE = m_CurlCookieHandle`.
  - Sets GET/POST/PUT/DELETE/HEAD according to `UtilHTTPMethod`.
  - Sets default request headers: `Host` and `User-Agent` (Chrome UA).

### 4) Sending and Driving (RunFrame pump)
- `IUtilHTTPRequest::Send()` only performs `curl_multi_add_handle(m_CurlMultiHandle, m_CurlEasyHandle)` and triggers `OnUpdateState(..., Requesting)` (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`).
- `IUtilHTTPClient::RunFrame()`:
  1. `curl_multi_perform()` advances transfers.
  2. `curl_multi_info_read()` reads completion messages; retrieves `CUtilHTTPRequest*` through `CURLINFO_PRIVATE` and calls `OnHTTPComplete()` (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`).
  3. Scans the request pool; if a request `IsFinished()` and `IsAutoDestroyOnFinish()`, calls `Destroy()` and removes it from the pool (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`).

### 5) Response Writes and Callback Order
- Header callback: `WriteHeaderCallback()` first calls `pRequest->OnRespondStart()` (sets the state to Responding and calls `OnUpdateState(..., Responding)`), then writes the header (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`).
- Body callback:
  - Standard requests: `WritePayloadCallback()` writes the chunk into `CUtilHTTPResponse::WritePayload()` (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`).
  - Streaming requests: `WritePayloadStreamCallback()` directly calls `OnReceiveData()`; before the first data arrives, it calls `FinalizeHeaders()` to ensure headers can be queried (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`).
- Completion: `CUtilHTTPRequest::OnHTTPComplete()` calls `FinalizeHeaders()`, `FinalizePayload()`, then `OnResponseComplete()`, and finally `OnRespondFinish()` -> `OnUpdateState(..., Finished)` (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`).

### 6) Request Pool (ID lifecycle)
- `AddToRequestPool()` assigns an incrementing ID to a request and writes it into `m_RequestPool` (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`).
- `GetRequestById()`/`DestroyRequestById()` provide cross-frame lookup and destruction (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`).

## Dependencies
### Code/Build-Time Dependencies
- C++20: Uses `std::format` (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`; the project also explicitly sets `LanguageStandard=stdcpp20`, see `PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.vcxproj`).
- Interface header: `include/Interface/IUtilHTTPClient.h` (the implementation depends on its interfaces and version macros).
- ScopeExit: `#include <ScopeExit/ScopeExit.h>` (used for `SCOPE_EXIT{ ... };`, see `PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`).
- libcurl: `#include <curl/curl.h>` (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`).

### Project Configuration Dependencies (vcxproj)
- Injected through MSBuild properties: `$(LibCurlIncludeDirectory)`, `$(LibCurlLibrariesDirectory)`, and `$(LibCurlLibraryFiles)`; the binary is copied in PostBuild (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.vcxproj`).

## Notes / Known Issues (Current Source Behavior)

1) “SyncRequest” is not self-driving.
- `CUtilHTTPSyncRequest::WaitForComplete()` only waits on the condition variable; the completion signal comes from the multi pump in `CUtilHTTPClient::RunFrame()` (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`). Therefore, either:
  - another thread must continuously call `RunFrame()`, or
  - avoid the blocking Wait and poll `IsFinished()` instead.
  - This behavior is expected.

2) Callback object ownership
- The `CUtilHTTPRequest` destructor calls `m_Callbacks->Destroy()` (`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`), which means the request owns the callback object's lifecycle; callers must allocate/implement callbacks according to this contract.

## Relationships (Caller Integration Clue)
- `Plugins/SCModelDownloader/UtilHTTPClient.cpp` contains logic that loads both `UtilHTTPClient_libcurl.dll` and `UtilHTTPClient_SteamAPI.dll` (obtaining factories by interface version string), serving as a reference for the library's actual plugin-side integration entry point.
