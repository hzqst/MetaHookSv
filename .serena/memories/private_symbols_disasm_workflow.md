# 通过反汇编引擎定位未导出私有函数/变量：通用 workflow（MetaHookSv）

目标：
- 在 **未导出**（无符号/无导出表）的情况下，稳定定位引擎/客户端 DLL 内部的私有函数与全局变量（含 vftable、数组基址、结构体指针等），并把结果写入 `gPrivateFuncs` 或全局指针，供后续 hook/patch 使用。

适用代码参考（本 repo 内典型实现）：
- `Plugins/Renderer/gl_hooks.cpp`：`Engine_FillAddress_R_RenderView`、`Engine_FillAddress_RenderSceneVars`
- `Plugins/BulletPhysics/privatehook.cpp`：`Engine_FillAddress_CL_ReallocateDynamicData`、`Engine_FillAddress_VisEdicts`、`Engine_FillAddress_CL_ViewEntityVars`
- `Plugins/VGUI2Extension/privatefuncs.cpp`：`VGUI2_FindMenuVFTable`、`VGUI2_FindKeyValueVFTable`、`Engine_FillAddress_Sys_InitializeGameDLL`

---

## 0) 前置输入与约束

1) 两份模块信息（非常关键）：
- `DllInfo`：用于 **特征码/字符串** 扫描的地址空间（通常是“扫描空间/分析空间”）。
- `RealDllInfo`：用于 **运行时调用/Hook** 的地址空间（真实装载基址）。

2) 你需要能区分 section 边界：
- `.text`：`TextBase/TextSize`
- `.data`：`DataBase/DataSize`
- `.rdata`：`RdataBase/RdataSize`

3) 引擎/版本分歧要提前纳入：
- 典型以 `g_iEngineType` 分支：`ENGINE_SVENGINE` / `ENGINE_GOLDSRC` / `ENGINE_GOLDSRC_BLOB` / `ENGINE_GOLDSRC_HL25`。

---

## 1) 选“锚点”（anchor）

优先级从高到低（越靠前越稳）：

### A. 强唯一字符串（推荐）
- 在 `.data/.rdata` 找到 **日志/报错字符串**（如 `"R_RenderView: NULL worldmodel"`、`"CL_Reallocate cl_entities\n"`、`"Sys_InitializeGameDLL called twice"`）。
- 用 `Search_Pattern_Data` / `Search_Pattern_Rdata` 定位字符串地址。

### B. 字符串 xref 代码形态
- 构造 “push 字符串地址 + call” 的小特征码，把字符串地址写进 pattern 的立即数区域，再 `Search_Pattern(pattern, DllInfo)`。
- 例：`push <str>; call <...>`、或带条件跳转的 `75 ?? 68 <str>`。

### C. 纯 `.text` 特征码（按引擎类型分治）
- 为不同引擎维护不同签名常量（SVEngine / HL25 / GoldSrc / Blob），并支持多套 fallback。
- 例：`Engine_FillAddress_R_RenderView` / `Engine_FillAddress_R_DrawTEntitiesOnList`。

### D. “结构写入”锚点（vftable/数组写入等）
- 先靠字符串找到构造/初始化附近，再靠反汇编识别：
  - vftable 写入：`mov [this], imm` 且 `imm` 位于 `.rdata`，并且 vftable[0] 指向 `.text`。
  - 数组写入：`mov [disp + index*scale], reg` 取 `disp` 作为数组基址。

---

## 2) 从锚点反推函数入口（Function Begin Recovery）

当锚点只是函数中间某条指令/某个调用点时：
- 用 `g_pMetaHookAPI->ReverseSearchFunctionBeginEx(anchor, max_back, predicate)` 反向找函数头。

predicate 常见写法（按编译器/版本多态）：
- 标准 prologue：`55 8B EC`
- 变体：`53 8B DC`（push ebx; mov ebx, esp）
- 变体：`8B 44 24 ?? 83 EC ??`（无 `push ebp` 的序言）
- 也会针对某些函数起始有特殊指令做判定（例如 `D9 05 ...` 开头等）。

---

## 3) 反汇编扫描（DisasmRanges / DisasmSingleInstruction）

核心套路：
- 对 **候选函数入口** 或 **锚点附近** 调用 `g_pMetaHookAPI->DisasmRanges(start, len, callback, ..., ctx)`。
- 在 callback 内用 Capstone 的 `cs_insn`（本项目里用 `pinst->id` + `pinst->detail->x86.operands[...]`）做语义匹配。
- 找到目标后尽快 `return TRUE` 停止扫描；同时也用 `RET`、`0xCC`、或 `instCount` 上限作为终止条件。

常见扫描窗口：
- 小窗口：0x30~0x100（定位单个变量/短调用链）
- 中窗口：0x150~0x500（定位多个变量或跨基本块）

---

## 4) 从指令语义抽取“目标地址”

下面这些判据在三个插件里反复出现，可直接复用：

### A. 抽取 call 目标（私有函数地址）
- 识别 `E8 rel32`（或 Capstone `X86_INS_CALL`），用：
  - `GetCallAddress(address)` 或
  - `pinst->detail->x86.operands[0].imm`（项目中也直接用 imm 当作 call target）。
- 常见用法：先在锚点附近找某个 `call`，再把 call target 当作“候选私有函数”。

### B. 抽取全局变量地址（绝对 disp / imm）
- 识别 `mov reg, [disp]`、`cmp [disp], imm`、`push [disp]` 等：
  - `operands[*].type == X86_OP_MEM` 且 `mem.base == 0 && mem.index == 0`（绝对地址）
  - 并校验 `mem.disp` 落在 `DllInfo.DataBase..DataBase+DataSize`（全局变量通常在 `.data`）
- 有时还会结合 **后继字节** 做更强语义校验（减少误判）：
  - 例如 `memcmp(address + instLen, "\x83\xC4\x04", 3)` 检查后面是 `add esp, 4`。

### C. 抽取数组基址（带 index*scale 的写入）
- 识别形如 `mov [disp + ecx*4], eax/edx`：
  - `operands[0].mem.index == X86_REG_ECX && mem.scale == 4`
  - `mem.disp` 即数组基址（如 `cl_visedicts`）。

### D. 定位 vftable（C++ 对象虚表）
- 常见模式：在 ctor 中看到 `mov [this + 0], imm`。
- 判定顺序：
  1) `imm` 落在 `.rdata` 范围（虚表常量在 rdata）
  2) 把 `imm` 视为 `PVOID* vftable`，要求 `vftable[0]` 指向 `.text`（虚函数入口在 text）
- 例：`VGUI2_FindMenuVFTable`、`VGUI2_FindKeyValueVFTable`。

### E. 通过“push 立即数参数”反推出结构体指针
- 观察 `push imm; call ...` 形式，`imm` 往往是某个全局结构体地址。
- 例：`Engine_FillAddress_RenderSceneVars` 里通过 call 前的 `push` 抽到 `refdef` 指针，并根据引擎类型映射到不同结构布局。

---

## 5) 地址空间映射：`ConvertDllInfoSpace`

扫描阶段拿到的地址通常属于 `DllInfo` 空间，真正使用/Hook 需要 `RealDllInfo` 空间：
- `ConvertDllInfoSpace(addr, DllInfo, RealDllInfo)`：
  - `RVA = RVA_from_VA(addr, SrcDllInfo)`
  - `VA = VA_from_RVA(RVA, TargetDllInfo)`

反向也会用到：
- 例如已知 `gPrivateFuncs.R_RenderScene`（Real 空间）但要在 `DllInfo` 空间继续扫描时，会先 `ConvertDllInfoSpace(real, RealDllInfo, DllInfo)`。

---

## 6) 兜底、缓存与可维护性

1) 缓存：
- 每个 `Engine_FillAddress_*` 入口先 `if (gPrivateFuncs.xxx) return;`，避免重复扫描。

2) 兜底顺序建议：
- 字符串锚点（稳定） -> 纯特征码（按引擎类型） -> 更宽的 disasm 语义匹配（代价大）。

3) 误判控制：
- 任何“从 disasm 抽到的地址”都要做区间校验（text/data/rdata）。
- 用 `instCount` 窗口限制（例如只允许前 N 条指令命中）。
- 用相邻字节/立即数语义（例如 cmp 的立即数等）做二次确认。

4) 扫描停止条件：
- 命中目标 / `RET` / `0xCC` / `instCount` 超限。

5) 结果校验与报错：
- 本 repo 习惯在填充后调用 `Sig_FuncNotFound(...)` / `Sig_VarNotFound(...)` / `Sig_AddrNotFound(...)` 做断言/日志。

---

## 7) 落地使用（典型调用链）

- `Engine_FillAddress(...)` 聚合调用多个 `Engine_FillAddress_*`：
  - 先定位私有函数/全局变量 -> 转换到运行时空间 -> 写入 `gPrivateFuncs` / 全局指针。
- 随后在 `*_InstallHooks()` 中基于这些地址安装 inline hook / vft hook 等（与本 memory 的定位流程解耦）。
