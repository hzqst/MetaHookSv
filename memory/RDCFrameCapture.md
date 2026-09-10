---
title: rdc-cli Frame Capture
type: note
permalink: metahooksv/rdc-cli-frame-capture
---

# rdc-cli Frame Capture

## Overview

`rdc-cli` is a scriptable CLI wrapper around RenderDoc that turns `.rdc` captures into Unix text streams (TSV by default, `--json` / `--jsonl` on request), so frames can be inspected with `grep` / `awk` / `jq` or by an AI agent. It is the terminal counterpart to the RenderDoc GUI procedures in [[multiview-depth-debugging]], [[wsurf-multiview-implementation]] and [[draw-scene]] — same captures, same replay engine, no GUI.

Typical use here: capture a frame with a [[renderer]] feature enabled, then inspect shaders, UBO contents, draw state and render targets from the command line.

## Environment

| Item | Value |
| --- | --- |
| CLI entry point | `rdc` (on `PATH` via `%USERPROFILE%\.local\bin`) |
| Source / editable install | `D:\rdc-cli` (`uv tool install -e .`) |
| RenderDoc Python module | **1.43**, built from `D:\renderdoc-fork` (x64 Release — the module itself is x64-only) |
| Module install dir | `%LOCALAPPDATA%\rdc\renderdoc` (a default rdc search path) |
| 32-bit components | `%LOCALAPPDATA%\rdc\renderdoc\x86\` — `renderdoccmd.exe`, `renderdocshim32.dll`, `renderdoc.dll`, 32-bit **1.43** (fork `Release\|Win32`); required for every GoldSrc / MetaHook target |
| Health check | `rdc doctor` — expect every line `[ok]` |

The module is built against **Python 3.13**. `rdc doctor` reports `win-python-version` as a failure if the running interpreter ever stops matching it.

`rdc capture` drives RenderDoc's `ExecuteAndInject`, i.e. **launch-time injection**. It cannot attach to an already-running game — the target must be started *by* `rdc`.

## Capturing a MetaHook Game

`metahook.exe` loads `hw.dll` / `sw.dll` into **its own process** (`src/launcher.cpp` reads the `EngineDLL` registry value and loads it in-process). The engine is never spawned as a child, so the capture target is `metahook.exe` itself and `--hook-children` is not needed.

RenderDoc sets the child's working directory to the executable's own directory when none is supplied (`os/win32/win32_process.cpp`, empty `workingDir` falls back to `get_dirname(app)`), which reproduces the launcher shortcut's `WorkDir` automatically. No `cd` is required.

```bash
rdc capture -o frame.rdc -- \
  "D:\SteamLibrary\steamapps\common\Half-Life\metahook.exe" -insecure -game cstrike
```

Substitute `-game` for the mod under test (`cstrike`, `valve`, `bshift`, `svencoop`, ...), matching the corresponding `MetaHook for *.lnk` shortcut. On success the capture path is printed to stdout and a `next: rdc open ...` hint goes to stderr.

Capture a chosen frame rather than the first one — necessary for shadow and multiview work, where the pass of interest is mid-frame:

```bash
rdc capture -o frame.rdc --frame 300 -- "<...>\metahook.exe" -insecure -game cstrike
rdc capture -o frame.rdc --auto-open  -- "<...>\metahook.exe" -insecure -game cstrike
```

### Options

| Option | Purpose |
| --- | --- |
| `-o, --output PATH` | Output `.rdc` path |
| `--frame N` | Queue the capture at frame N |
| `--trigger` | Inject only, no auto-capture; prints `ident=N` for a later `rdc attach` |
| `--keep-alive` | Leave the target running after capture |
| `--wait-for-exit` | Block until the target process exits |
| `--auto-open` | Open the capture when done |
| `--timeout SEC` | Capture timeout (default 60) |
| `--hook-children` | Also hook child processes (needed only for launcher-style startup, not for `metahook.exe`) |
| `--callstacks` | Record CPU callstacks |
| `--json` | Machine-readable result |

Manual trigger flow, for capturing a specific in-game moment:

```bash
rdc capture --trigger -- "<...>\metahook.exe" -insecure -game cstrike   # prints ident=N
rdc attach N
rdc capture-trigger
rdc capture-list
rdc capture-copy 0 frame.rdc
```

## 32-bit Targets

Every MetaHook game and the standalone Steam `svencoop.exe` are **PE32 (32-bit)**. RenderDoc captures an alt-bitness target by farming off to a `renderdoccmd` of the *target's* bitness (the `capaltbit` command), which must sit next to the module:

| File under the module dir | Role |
| --- | --- |
| `x86\renderdoccmd.exe` | alt-bit launcher; runs `capaltbit`, injects from its own directory |
| `x86\renderdocshim32.dll` | shim loaded into the target |
| `x86\renderdoc.dll` | the injected 32-bit RenderDoc |

The 1.43 module is an **x64-only** build — `rdc setup-renderdoc` stages only `renderdoc.pyd`, `renderdoc.dll` and `renderdoccmd.exe` (x64) — so `x86\` is absent by default and every 32-bit attempt fails with a misleading error:

```
inject failed (code <Internal error: Can't run 32-bit renderdoccmd to capture 32-bit program.
If this is a locally built RenderDoc you must build both 32-bit and 64-bit versions.>)
```

That text is emitted whenever `CreateProcessW` of the alt-bit `renderdoccmd` fails (`renderdoc/os/win32/win32_process.cpp`); the real cause is the missing `x86\` files, not a bitness mismatch in `renderdoccmd` itself.

### Build and install

The fork's `Win32\Release` already has `renderdoc.dll` and `qrenderdoc.exe` but not `renderdoccmd` / `renderdocshim32.dll`. Build just those two (`renderdoc.lib` and the drivers are already there):

```bash
MSBUILD="/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe"
export MSYS2_ARG_CONV_EXCL='*'                 # Git Bash rewrites /p:... into paths
cd /d/renderdoc-fork
# use -p: / -m (dash), not /p: / /m — MSYS turns leading-slash switches into filenames
"$MSBUILD" renderdocshim/renderdocshim.vcxproj -p:Configuration=Release -p:Platform=Win32 -p:SolutionDir=D:/renderdoc-fork/ -m -v:m
"$MSBUILD" renderdoccmd/renderdoccmd.vcxproj   -p:Configuration=Release -p:Platform=Win32 -p:SolutionDir=D:/renderdoc-fork/ -m -v:m
```

Installing `renderdoccmd` relinks `Win32/Release/renderdoc.dll` as a dependency, so all three outputs end up same-source. Copy them in:

```bash
cp D:/renderdoc-fork/Win32/Release/{renderdoccmd.exe,renderdocshim32.dll,renderdoc.dll} \
   "$LOCALAPPDATA/rdc/renderdoc/x86/"
```

Verify each is `PE32` (not `PE32+`) and that `renderdoccmd.exe version` reports `x86 v1.43`.

### Path resolution

For the installed (non-development) layout the x64 module derives the alt-bit paths from its own directory: it strips the filename and appends `x86\renderdoccmd.exe` (and `x86\renderdocshim32.dll`), so placing the files there is the whole configuration — no env var, no config file. The dev-layout branches (`\x64\Release\` → `\Win32\Release\`) never trigger for `%LOCALAPPDATA%\rdc\renderdoc`.

## Inspecting a Capture

```bash
rdc open frame.rdc      # starts the session daemon
rdc info
rdc close               # release when finished
```

Everything below needs an open session. Output is TSV; headers and summaries go to stderr so stdout stays pipe-clean.

| GUI workflow (see [[multiview-depth-debugging]]) | rdc equivalent |
| --- | --- |
| Event browser — find draw calls | `rdc draws`, `rdc events` |
| Pass overview, dead render targets | `rdc passes`, `rdc pass <n>`, `rdc unused-targets` |
| Pipeline state at a draw | `rdc pipeline <EID>`, `rdc draw <EID>` |
| Shader metadata, source, bindings | `rdc shader <EID> <stage>`, `rdc shader <EID> --all`, `rdc bindings <EID>` |
| Checkpoint 1 — validate CameraUBO | `rdc cbuffer <EID> --stage <stage> --set <s> --binding <b>` |
| Checkpoint 2 — compare `gl_Position`, multiview vs not | capture both, then `rdc diff a.rdc b.rdc --shortstat` |
| Checkpoint 3 — inspect `gl_Layer` / array slices | `rdc texture <id> -o out.png`, `rdc rt <EID> -o rt.png` |
| Pixel history, shader debugging | `rdc pixel X Y`, `rdc pick-pixel X Y`, `rdc debug pixel <EID> X Y --trace` |
| Search shader disassembly | `rdc search "<regex>"` |

Constant buffers are also reachable through the VFS, which is the reliable way to discover the right `--set` / `--binding` values:

```bash
rdc ls /draws/<EID>/cbuffer/<stage>          # descriptor sets
rdc ls /draws/<EID>/cbuffer/<stage>/<set>    # bindings
rdc cbuffer <EID> --stage vs --set 0 --binding 0
```

Shader edit-replay — hot-swap a shader without restarting the game:

```bash
rdc shader <EID> ps --source > s.frag
# ... edit ...
rdc shader-build s.frag --stage ps           # prints the built shader ID
rdc shader-replace <EID> ps --with <ID>
rdc shader-restore <EID> ps                  # STAGE is required
```

CI-style assertions use diff(1)-compatible exit codes (`0`=pass, `1`=fail, `2`=error): `assert-pixel`, `assert-state`, `assert-image`, `assert-count`, `assert-clean`.

## Troubleshooting

- **Run `rdc doctor` first** — it reports module, renderdoccmd, adb and platform toolchain status.
- **`ident=0` / injection failed** — run the terminal as Administrator; some drivers and overlay software block injection.
- **`inject failed ... Can't run 32-bit renderdoccmd to capture 32-bit program`** — the module's `x86\` folder is missing or incomplete; the message is misleading (it is not about `renderdoccmd`'s own bitness). Build and install the 32-bit components — see [32-bit Targets](#32-bit-targets).
- **Anti-cheat games cannot be captured.** EAC / BattlEye / Vanguard reject injection by design. The `-insecure` MetaHook launchers and their mods are unaffected.
- **`--frame N` times out** — raise `--timeout`, or fall back to `--trigger` plus a manual trigger.
- **`no constant block at set=... binding=...`** — that stage has no UBO at those indices; enumerate them with `rdc ls /draws/<EID>/cbuffer/<stage>` first.
- **Vulkan captures are not enabled in this install.** `renderdoc.json` was deliberately not placed in the module dir, so `rdc capture` does not set `ENABLE_VULKAN_RENDERDOC_CAPTURE` / `VK_IMPLICIT_LAYER_PATH`. This does **not** affect MetaHook (OpenGL, or D3D9 via `-d3d`). A Vulkan target needs that file copied from the fork's `x64\Release\`.

## Notes

- The GUI RenderDoc 1.44 at `C:\Program Files\RenderDoc` is a **separate installation** with its own `VK_LAYER_RENDERDOC_Capture` layer registered. `rdc` uses the 1.43 module in `%LOCALAPPDATA%\rdc\renderdoc`; do not mix the two in one session.
- `rdc script` runs arbitrary Python inside the daemon when a needed query has no CLI command.
- Long operations (large capture transfer, remote replay init) have limited progress output — a known UX gap, not a hang.
- `rdc setup-renderdoc` builds the Python module from source. On this machine it required a patch: `_vs_install_path()` in `src/rdc/_build_renderdoc.py` must pass `-latest` to `vswhere`, otherwise VS 2019 BuildTools is selected ahead of VS 2022 Community and the build dies with `MSB8020` (v143 toolset not found).
