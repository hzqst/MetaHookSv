# UtilHTTPClient_Steam（实际目录：PluginLibs/UtilHTTPClient_SteamAPI）源码分析

## 概述
`PluginLibs/UtilHTTPClient_SteamAPI` 是一个基于 Steamworks `ISteamHTTP`（`steam_api.h`）实现的 HTTP 客户端实现库，向项目内其它模块提供统一的 `IUtilHTTPClient`/`IUtilHTTPRequest`/`IUtilHTTPResponse` 接口（见 `include/Interface/IUtilHTTPClient.h`），并通过 `EXPOSE_INTERFACE`/`EXPOSE_SINGLE_INTERFACE` 导出接口版本：
- `UTIL_HTTPCLIENT_STEAMAPI_INTERFACE_VERSION` = `UtilHTTPClient_SteamAPI_007`
- `UTIL_HTTPCLIENT_FACTORY_STEAMAPI_INTERFACE_VERSION` = `UtilHTTPClientFactory_SteamAPI_007`

核心实现集中在：
- `PluginLibs/UtilHTTPClient_SteamAPI/UtilHTTPClient_SteamAPI.cpp`

## 职责
- 提供 HTTP 请求创建与发送：同步请求（可阻塞等待）、异步请求（回调驱动）、异步流式请求（按块接收）。
- 对 Steam HTTP API 做一层抽象：方法映射、header 设置、post body 设置、证书校验开关、cookie 容器。
- 为异步请求提供“请求池 + 每帧清理”机制（`RunFrame()`），避免调用方自行管理生命周期。

## 架构
### 接口层（`include/Interface/IUtilHTTPClient.h`）
- `IUtilHTTPClient`
  - `Init/Shutdown/RunFrame`
  - `CreateSyncRequest/CreateAsyncRequest/CreateAsyncStreamRequest`
  - `AddToRequestPool/GetRequestById/DestroyRequestById`
  - `SetCookie`
- `IUtilHTTPRequest`
  - `Send`、header/body/timeout 等设置
  - 状态查询（`UtilHTTPRequestState`）
  - 同步请求的 `WaitForComplete/WaitForCompleteTimeout/GetResponse`
- `IUtilHTTPResponse`
  - status code、headers、payload
- `IUtilHTTPCallbacks`
  - `OnUpdateState/OnReceiveData/OnResponseComplete` + `Destroy()`
- `IURLParsedResult`
  - URL 解析结果载体（scheme/host/port/target/secure）

### 实现层（`PluginLibs/UtilHTTPClient_SteamAPI/UtilHTTPClient_SteamAPI.cpp`）
- `UTIL_ConvertUtilHTTPMethodToSteamHTTPMethod`：`UtilHTTPMethod -> EHTTPMethod` 映射。
- `ParseUrlInternal` + `CURLParsedResult`：URL 解析（返回 `IURLParsedResult*`）。
- `CUtilHTTPPayload`：封装响应 body（非流式时读取完整 body）。
- `CUtilHTTPResponse`：封装响应状态码、header（带缓存）、payload；与 Steam 回调对接。
- `CUtilHTTPRequest`：请求基类（实现 `IUtilHTTPRequest`），持有 `HTTPRequestHandle` 与 Steam `CCallResult`：
  - `SendHTTPRequest` 触发请求
  - `OnSteamHTTPHeaderReceived/OnSteamHTTPCompleted` 驱动状态迁移与回调
  - 析构中取消 `CallResult`、`ReleaseHTTPRequest`、并调用 `m_Callbacks->Destroy()`
- `CUtilHTTPSyncRequest`：同步请求实现
  - 用 `condition_variable` 等待 `OnSteamHTTPCompleted` 设置完成标记
  - `GetResponse()` 返回 `m_pResponse`
- `CUtilHTTPAsyncRequest`：异步请求实现
  - 默认 `m_bAutoDestroyOnFinish = true`
  - `WaitForComplete/WaitForCompleteTimeout` 为 stub，`GetResponse()` 直接返回 `nullptr`
- `CUtilHTTPAsyncStreamRequest`：异步流式请求实现
  - 使用 `SendHTTPRequestAndStreamResponse`
  - 通过 `HTTPRequestDataReceived_t` 取回分块数据并尝试回调 `OnReceiveData`
- `CUtilHTTPClient`：HTTP Client 实例
  - 可选创建 cookie 容器（`CreateCookieContainer`）
  - 请求池：`unordered_map<UtilHTTPRequestId_t, IUtilHTTPRequest*> m_RequestPool`
  - `RunFrame()` 清理：若请求 `IsFinished && IsAutoDestroyOnFinish` 则 `Destroy + erase`
- `CUtilHTTPClientFactory`：工厂
  - `CreateUtilHTTPClient()` 返回 `new CUtilHTTPClient`
  - `ParseUrl()` 直接复用 `ParseUrlInternal`

## 核心实现与 workflow
### 1) Client 初始化/循环
1. 通过工厂导出接口创建 `IUtilHTTPClient`（版本 `*_007`）。
2. `IUtilHTTPClient::Init(context)`：若 `context->m_bUseCookieContainer`，创建 `HTTPCookieContainerHandle`。
3. 每帧调用 `IUtilHTTPClient::RunFrame()`：清理由请求池托管、且完成后自动销毁的请求。

注意：Steam 的 `CCallResult` 回调触发依赖外部定期执行 Steamworks 的回调泵（通常是 `SteamAPI_RunCallbacks()`）；否则同步等待可能卡死。

### 2) 同步请求（Sync）
- 调用：`CreateSyncRequest(url, method, callbacks)`
  - 内部 `ParseUrlInternal(url)` -> `CreateSyncRequestEx(host, port, target, secure, ...)` -> `new CUtilHTTPSyncRequest`。
- 配置：`SetTimeout/SetField/SetPostBody/SetRequireCertVerification`。
- 发送：`Send()`
  - `SteamHTTP()->SendHTTPRequest` + 绑定 `m_CompleteCallResult/m_HeaderReceivedCallResult`
  - 立即触发 `callbacks->OnUpdateState(..., Requesting)`
- 等待：`WaitForComplete()` 或 `WaitForCompleteTimeout()`
  - 由 `CUtilHTTPSyncRequest::OnSteamHTTPCompleted` 设置完成标记并唤醒。
- 取结果：`GetResponse()` -> `IUtilHTTPResponse`
  - 非流式：完成时会读取 body 到 `CUtilHTTPPayload`。

### 3) 异步请求（Async）
- 调用：`CreateAsyncRequest(url, method, callbacks)` -> `new CUtilHTTPAsyncRequest`（默认自动销毁）。
- 发送：`Send()` 与同步相同，但无阻塞等待。
- 生命周期：
  - 调用方要么：`client->AddToRequestPool(request)` 让 client 托管（建议），并用 request id 查询；
  - 要么：自己持有并在完成后 `Destroy()`。

### 4) 异步流式请求（AsyncStream）
- 发送使用 `SteamHTTP()->SendHTTPRequestAndStreamResponse`，并监听：
  - `HTTPRequestHeadersReceived_t`：切入 Responding
  - `HTTPRequestDataReceived_t`：读取 chunk，并调用 `IUtilHTTPCallbacks::OnReceiveData`
  - `HTTPRequestCompleted_t`：完成回调

## 依赖
- Steamworks SDK：
  - include：`include/SteamSDK`（见 `PluginLibs/UtilHTTPClient_SteamAPI/UtilHTTPClient_SteamAPI.vcxproj`）
  - link：`steam_api.lib`
  - 代码依赖：`steam_api.h`、`SteamHTTP()`、`CCallResult<...>`
- C++20：`std::format`（`<format>`）与 `std::regex`（`<regex>`）；项目显式设置 `LanguageStandard=stdcpp20`。
- ScopeExit：`<ScopeExit/ScopeExit.h>`（工程里通过 `$(ScopeExitIncludeDirectory)` 提供 include path）。
- Windows：`<Windows.h>`
- 本项目接口：`include/Interface/IUtilHTTPClient.h`

## 注意事项 / 风险点
3. `CUtilHTTPAsyncRequest` 的 `WaitForComplete/WaitForCompleteTimeout/GetResponse` 为 stub/返回 `nullptr`：异步请求必须通过回调消费结果。这是符合预期的行为。
5. callback 生命周期约定：`CUtilHTTPRequest` 析构会调用 `m_Callbacks->Destroy()`。因此 callbacks 必须是“由请求拥有”的对象（通常堆分配），且 `Destroy()` 需自删除；否则会产生 double free/释放栈对象等风险。
6. `SetFollowLocation` 在 SteamAPI 后端未实现（直接 `//not supported`）。
7. `CUtilHTTPResponse::GetResponseErrorMessage()` 返回的 `m_ResponseErrorMessage` 从未被填充，错误信息可能永远为空；仅能靠 `IsResponseError/IsRequestSuccessful/StatusCode` 判断。因为SteamAPI未提供错误信息。
8. 同步等待依赖 Steam 回调泵：如果外部没有持续 `SteamAPI_RunCallbacks()`，`WaitForComplete()` 可能永久阻塞。这是符合预期的行为。
10. 默认 `User-Agent` 硬编码为 Chrome UA；某些服务端策略可能依赖 UA（可通过 `SetField` 覆盖）。
