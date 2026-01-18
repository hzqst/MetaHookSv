# UtilHTTPClient_libcurl（源码级分析）

## 概述
`PluginLibs/UtilHTTPClient_libcurl` 是一个基于 **libcurl（multi/easy）** 的 HTTP 客户端实现，作为 MetaHookSv 的“工具库”以 DLL 形式导出 `IUtilHTTPClient` 与 `IUtilHTTPClientFactory` 接口（见 `PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）。

该实现的设计目标是：
- 在引擎/插件主循环里每帧驱动网络请求（`IUtilHTTPClient::RunFrame()`）。
- 支持同步/异步请求，以及“流式接收”（chunk 回调）。
- 可选 Cookie 容器（跨请求共享 Cookie）。

## 职责（Responsibilities）
- **IUtilHTTPClient 实现**：创建/管理请求对象、驱动 libcurl multi、提供请求池（id -> request）生命周期管理。
- **请求抽象**：将 `IUtilHTTPRequest` 的 API（`Send/SetTimeout/SetPostBody/SetField/...`）映射到 libcurl easy 选项。
- **响应抽象**：把 header/body 收集到 `IUtilHTTPResponse`/`IUtilHTTPPayload`，并提供 header 查询。
- **回调分发**：通过 `IUtilHTTPCallbacks` 推送状态变化、流式数据、完成事件。

## 架构
核心类型与关系（均在 `PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）：
- `CUtilHTTPClient`（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）：
  - 持有 `CURLM* m_CurlMultiHandle`（multi）、可选 `CURLSH* m_CurlCookieHandle`（share-cookie）。
  - 维护请求池：`unordered_map<UtilHTTPRequestId_t, IUtilHTTPRequest*> m_RequestPool` + `m_RequestHandleLock`。
  - `RunFrame()` 驱动 `curl_multi_perform()` 并处理 `CURLMSG_DONE`，然后清理 auto-destroy 请求。
- `CUtilHTTPRequest`（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）：
  - 基类：封装一个 `CURL* m_CurlEasyHandle`，并将 libcurl 回调（header/body）转发给 `CUtilHTTPResponse`。
  - 持有 `CUtilHTTPResponse* m_pResponse` 和 `IUtilHTTPCallbacks* m_Callbacks`（析构会 `m_Callbacks->Destroy()`）。
- `CUtilHTTPSyncRequest`（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）：
  - 在 `OnRespondFinish()` 里置位并 `notify_one()`，为 `WaitForComplete()` 提供条件变量。
- `CUtilHTTPAsyncRequest`（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）：
  - 纯异步外壳；`WaitForComplete/GetResponse` 为空实现/返回 `nullptr`。
- `CUtilHTTPAsyncStreamRequest`（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）：
  - 覆盖 easy 的 write callback，直接在 chunk 到达时调用 `IUtilHTTPCallbacks::OnReceiveData()`。
- `CUtilHTTPResponse`（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）：
  - header/body 各自一份 `CUtilHTTPPayload` 缓冲；`FinalizeHeaders()` 解析 header 行写入 `m_headers`。
- `CUtilHTTPPayload`（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）：
  - 用 `stringstream` 聚合数据，`Finalize()` 后将其固化到 `std::string m_payload`。
- `CURLParsedResult`/`ParseUrlInternal`（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）：
  - URL 解析结果对象（实现 `IURLParsedResult`）。

接口定义见：`include/Interface/IUtilHTTPClient.h`。

## 核心实现与 Workflow
### 1) 模块导出与创建
- 通过 `EXPOSE_INTERFACE(CUtilHTTPClient, IUtilHTTPClient, UTIL_HTTPCLIENT_LIBCURL_INTERFACE_VERSION)` 导出客户端（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）。
- 通过 `EXPOSE_SINGLE_INTERFACE(CUtilHTTPClientFactory, IUtilHTTPClientFactory, UTIL_HTTPCLIENT_FACTORY_LIBCURL_INTERFACE_VERSION)` 导出工厂（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）。
- 版本宏定义在 `include/Interface/IUtilHTTPClient.h`（如 `UTIL_HTTPCLIENT_LIBCURL_INTERFACE_VERSION`）。

### 2) 初始化/退出（multi + cookie share）
- `CUtilHTTPClient::Init(context)`：
  - 若 `context->m_bUseCookieContainer` 为真，则 `curl_share_init()` 并 `curl_share_setopt(..., CURL_LOCK_DATA_COOKIE)`（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）。
  - 创建 `m_CurlMultiHandle = curl_multi_init()`。
- `CUtilHTTPClient::Shutdown()`：
  - 销毁请求池中所有 request；cleanup multi 与 share（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）。

### 3) 创建请求（easy 句柄配置）
- `CUtilHTTPRequest` 构造时：
  - `curl_easy_init()`，构造 `CUtilHTTPResponse(m_CurlEasyHandle)`。
  - 拼接 URL：`std::format("{}://{}:{}{}", ...)`。
  - 绑定回调：`CURLOPT_WRITEFUNCTION/WRITEDATA`、`CURLOPT_HEADERFUNCTION/HEADERDATA`（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）。
  - 默认网络参数：`CONNECTTIMEOUT_MS/TIMEOUT_MS=60000`，`CURLOPT_ACCEPT_ENCODING=""` 启用解压，`CURLOPT_COOKIEFILE=""` 启用 cookie engine。
  - 如果启用 cookie share：`CURLOPT_SHARE = m_CurlCookieHandle`。
  - 根据 `UtilHTTPMethod` 设置 GET/POST/PUT/DELETE/HEAD。
  - 设置默认请求头：`Host`、`User-Agent`（Chrome UA）。

### 4) 发送与驱动（RunFrame pump）
- `IUtilHTTPRequest::Send()`：仅做 `curl_multi_add_handle(m_CurlMultiHandle, m_CurlEasyHandle)`，并触发 `OnUpdateState(..., Requesting)`（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）。
- `IUtilHTTPClient::RunFrame()`：
  1. `curl_multi_perform()` 推进传输。
  2. `curl_multi_info_read()` 读取完成消息；用 `CURLINFO_PRIVATE` 取回 `CUtilHTTPRequest*` 并调用 `OnHTTPComplete()`（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）。
  3. 扫描请求池，若 request `IsFinished()` 且 `IsAutoDestroyOnFinish()`，则 `Destroy()` 并从池移除（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）。

### 5) 响应写入与回调时序
- Header 回调：`WriteHeaderCallback()` 会先 `pRequest->OnRespondStart()`（状态置 Responding，并 `OnUpdateState(..., Responding)`），再写 header（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）。
- Body 回调：
  - 普通请求：`WritePayloadCallback()` 将 chunk 写入 `CUtilHTTPResponse::WritePayload()`（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）。
  - 流式请求：`WritePayloadStreamCallback()` 直接 `OnReceiveData()`；并在第一次数据到来前调用 `FinalizeHeaders()` 确保 header 可查（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）。
- 完成：`CUtilHTTPRequest::OnHTTPComplete()` 调用 `FinalizeHeaders()`、`FinalizePayload()`，随后 `OnResponseComplete()`，最后 `OnRespondFinish()` -> `OnUpdateState(..., Finished)`（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）。

### 6) 请求池（id 生命周期）
- `AddToRequestPool()` 给 request 分配自增 id，并写入 `m_RequestPool`（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）。
- `GetRequestById()`/`DestroyRequestById()` 提供跨帧检索与销毁（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）。

## 依赖
### 代码/编译期依赖
- C++20：使用 `std::format`（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`；项目也显式设置 `LanguageStandard=stdcpp20`，见 `PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.vcxproj`）。
- 接口头：`include/Interface/IUtilHTTPClient.h`（实现依赖其接口与版本宏）。
- ScopeExit：`#include <ScopeExit/ScopeExit.h>`（用于 `SCOPE_EXIT{ ... };`，见 `PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）。
- libcurl：`#include <curl/curl.h>`（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）。

### 工程配置依赖（vcxproj）
- 通过 MSBuild 属性注入：`$(LibCurlIncludeDirectory)`、`$(LibCurlLibrariesDirectory)`、`$(LibCurlLibraryFiles)`，并在 PostBuild 中拷贝 bin（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.vcxproj`）。

## 注意事项 / 已知问题（源码现状）

1) “SyncRequest” 并非自驱动
- `CUtilHTTPSyncRequest::WaitForComplete()` 只是等待条件变量；完成信号来自 `CUtilHTTPClient::RunFrame()` 的 multi pump（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`）。因此需要：
  - 另一线程持续调用 `RunFrame()`，或
  - 不使用阻塞 Wait，改为轮询 `IsFinished()`。
  - 这是符合预期的行为

2) 回调对象所有权
- `CUtilHTTPRequest` 析构会调用 `m_Callbacks->Destroy()`（`PluginLibs/UtilHTTPClient_libcurl/UtilHTTPClient_libcurl.cpp`），意味着回调对象的生命周期由 request 接管；调用方需要按此约定分配/实现 callbacks。

## 关联（调用方集成线索）
- `Plugins/SCModelDownloader/UtilHTTPClient.cpp` 存在同时加载 `UtilHTTPClient_libcurl.dll` 与 `UtilHTTPClient_SteamAPI.dll` 的逻辑（通过接口版本字符串取 factory），可作为该库在插件侧的实际集成入口参考。
