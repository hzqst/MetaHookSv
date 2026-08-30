---
title: UtilAssetsIntegrity
type: note
permalink: metahooksv/util-assets-integrity
---

# Source-Level Analysis of UtilAssetsIntegrity (`PluginLibs/UtilAssetsIntegrity`)

## Overview
`UtilAssetsIntegrity` is a standalone DLL (`UtilAssetsIntegrity.dll`) that performs basic integrity and bounds validation on binary assets from untrusted sources through the `IUtilAssetsIntegrity` interface, preventing malformed data from causing OOB reads/writes or crashes during parsing, rendering, or loading in upper layers.

The current implementation covers two resource types:
- GoldSrc/HL1 `StudioModel` (`IDST` main models and `IDSQ` sequence groups)
- 8-bit (indexed-color) BMP files (decoded with FreeImage and constrained by dimensions)

## Responsibilities
- Provides stable public APIs:
  - `CheckStudioModel(const void* buf, size_t bufSize, UtilAssetsIntegrityCheckResult_StudioModel* out)`
  - `Check8bitBMP(const void* buf, size_t bufSize, UtilAssetsIntegrityCheckResult_BMP* out)`
- Returns `UtilAssetsIntegrityCheckReason` (`OK/InvalidFormat/SizeTooLarge/SizeTooSmall/BogusHeader/VersionMismatch/OutOfBound/Unknown`) and attempts to populate `out->ReasonStr` with a readable reason.
- Is dynamically loaded by plugins/tools as a pre-download/import filter.

## Public Interface (`include/Interface`)
- Header: `include/Interface/IUtilAssetsIntegrity.h`
  - `UTIL_ASSETS_INTEGRITY_INTERFACE_VERSION` = `"UtilAssetsIntegrityAPI_001"` (`include/Interface/IUtilAssetsIntegrity.h:61`)
  - `IUtilAssetsIntegrity` inherits `IBaseInterface` (`include/Interface/IUtilAssetsIntegrity.h:51`)
  - `UtilAssetsIntegrityCheckResult`: contains only `ReasonStr[256]` (`include/Interface/IUtilAssetsIntegrity.h:17`)
  - `UtilAssetsIntegrityCheckResult_BMP`: adds constraint fields `MaxWidth/MaxHeight/MaxSize` (`include/Interface/IUtilAssetsIntegrity.h:34`)

## Architecture
- Code location: `PluginLibs/UtilAssetsIntegrity/`
  - `dllmain.cpp`: stub `DllMain` (`PluginLibs/UtilAssetsIntegrity/dllmain.cpp:2`)
  - `UtilAssetsIntegrity.cpp`: core implementation (the vast majority of the logic)
- Implementation class: `CUtilAssetsIntegrity : public IUtilAssetsIntegrity` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:24`)
- Export mechanism: uses the interface-registration macro `EXPOSE_SINGLE_INTERFACE(...)` from `metahook.h` to export the `CreateInterface` factory (near `PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:1159`).

## Core Implementation and Workflow

### 1) StudioModel Validation (IDST/IDSQ)
Entry point: `CUtilAssetsIntegrity::CheckStudioModel` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:1069`)
- Steps:
  1. `bufSize < sizeof(studiohdr_t)` => `SizeTooSmall`
  2. Reads the first four magic bytes:
     - `IDSQ` => `CheckStudioModel_IDSQ` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:1056`)
     - `IDST` => `CheckStudioModel_IDST` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:958`)
     - Otherwise => `BogusHeader`

#### 1.1) IDSQ (Sequence Group)
`CheckStudioModel_IDSQ` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:1056`)
- Checks only `studiohdr->version == 10`; otherwise returns `VersionMismatch`.

#### 1.2) IDST (Main Model)
`CheckStudioModel_IDST` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:958`)
- Key strategy: distinguishes the “system-memory segment (`system_memory_length`)” from the “complete file (`bufSize`)”
  - If `studiohdr->textureindex != 0`, uses `texturedataindex` as the system-memory upper bound (the end of the model-main segment)
  - Otherwise, uses `studiohdr->length` as the system-memory upper bound
  - Rationale: many structures/indices should reside in the model-main segment, while texture pixel data may reside at the end of the file
- Validation pipeline (returns immediately on failure at any step):
  - `version == 10`
  - Fields such as `texturedataindex` and `length` must be within the valid `[0, bufSize]` range
  - Validates each block as needed:
    - `numtextures` => `CheckStudioModel_Textures` (structure table) + `CheckStudioModel_TextureData` (pixel data)
    - `numskinfamilies && numskinref` => `CheckStudioModel_Skins`
    - `numbodyparts` => `CheckStudioModel_BodyParts` (recurses into model/mesh/tri commands)
    - `numbones` => `CheckStudioModel_Bones`
    - `numseq` => `CheckStudioModel_Sequences` (recurses into events / anim data)
    - `numhitboxes` => `CheckStudioModel_Hitboxes`
    - `numbonecontrollers` => `CheckStudioModel_BoneControllers`

#### 1.3) Structure-Block Details (Key Points)
- Texture table (`mstudiotexture_t`)
  - `CheckStudioModel_Textures` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:29`):
    - `numtextures` range (including the upper-bound constant)
    - `textureindex` within `[0, system_memory_length]`
    - `ptexture_base/end` within `[buf, buf+system_memory_length]`
  - `CheckStudioModel_TextureData` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:74`):
    - Each `ptexture[i].index` is within `[0, bufSize]`
    - `width/height` are nonnegative, and the `width*height` pixel segment `pal..pal+palsize` is within `[buf, buf+bufSize]`
    - `ptexture[i].name` must be NUL-terminated (using `safe_strlen`)

- Skin references (`skinindex` / `short` index matrix)
  - `CheckStudioModel_Skins` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:129`):
    - The `pskinref` table is within `[buf, buf+system_memory_length]`
    - Each `ref` satisfies `0 <= ref < numtextures`

- Deep validation of BodyParts -> Submodel -> Mesh -> tri commands
  - `CheckStudioModel_BodyParts` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:739`)
  - `CheckStudioModel_BodyPart` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:678`)
  - `CheckStudioModel_Submodel` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:575`)
  - `CheckStudioModel_Mesh` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:449`)
  - Core checks:
    - All indices (`bodypartindex/modelindex/meshindex/triindex/...`) must be nonnegative and not exceed `bufSize`
    - The pointer range `base..end` must fall within the buffer range
    - tri-command decoding: reads a `short` sequence; `t==0` terminates, `t<0` is a fan, and `t>0` is a strip; each vertex entry advances with `ptricmds += 4`
    - Protects `vertindex/normindex` against negative values and pointer OOB access (vertex, normal, and bone-index arrays)

- Bones / controllers / hitboxes
  - `CheckStudioModel_Bones` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:832`) + `CheckStudioModel_Bone` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:794`)
    - Bone names are NUL-terminated
    - `parent` must be `-1` or within `[0, numbones)`
    - `bonecontroller[j]` must be `-1` or within a valid range (see “Notes / Potential Issues”)
  - `CheckStudioModel_Hitboxes` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:406`) + `CheckStudioModel_Hitbox` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:389`)
    - Validates hitbox-table position, and `pbbox->bone` must be within `[0, numbones)`
  - `CheckStudioModel_BoneControllers` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:903`) + `CheckStudioModel_BoneController` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:886`)
    - Validates controller-table position, and `pbonecontroller->bone` must be valid

- Sequences / events / anim
  - `CheckStudioModel_Sequences` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:334`)
  - `CheckStudioModel_SeqDesc` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:307`)
  - `CheckStudioModel_SeqDescEvents` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:199`) + `CheckStudioModel_Event` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:188`)
    - Validates events-table position
    - `pevent->options` is NUL-terminated
  - `CheckStudioModel_SeqDescAnim` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:255`)
    - `animindex` is valid
    - `panim_base..panim_base+numbones` does not go out of bounds
    - Calculates `panimvalue` for `panim->offset[j+3]` (checks only the three rotation channels) and uses `(panimvalue + 255)` for approximate upper-bound validation

### 2) 8-bit BMP Validation
Entry point: `CUtilAssetsIntegrity::Check8bitBMP` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:1096`)
- Workflow:
  1. `FreeImage_OpenMemory(buf)`
  2. `FreeImage_LoadFromMemory(FIF_BMP, ...)`
  3. Must be `FIC_PALETTE` (indexed color), otherwise `InvalidFormat`
  4. If `checkResult` is supplied, uses its `MaxWidth/MaxHeight/MaxSize` as hard limits; exceeds => `SizeTooLarge`
  5. All checks pass => `OK`

Note: the “size limits” here depend entirely on the caller populating `checkResult->Max*`; see the next section for caller examples.

## Dynamic Loading and Typical External Caller Usage
This DLL is normally loaded at runtime by plugins, which acquire its instance through the interface version:
- `Plugins/SCModelDownloader/UtilAssetsIntegrity.cpp:12`:
  - `Sys_LoadModule("UtilAssetsIntegrity.dll")`
  - `Sys_GetFactory(hModule)`
  - `factory(UTIL_ASSETS_INTEGRITY_INTERFACE_VERSION, NULL)` obtains `IUtilAssetsIntegrity*`
- Upper layers call it before downloading/persisting or decoding assets:
  - `Plugins/SCModelDownloader/SCModelDatabase.cpp:520` sets BMP `MaxWidth/MaxHeight/MaxSize`, then calls `Check8bitBMP`
  - `Plugins/SCModelDownloader/SCModelDatabase.cpp:504` uses `CheckStudioModel`

## Dependencies
The primary build/header dependencies are:
- HLSDK / GoldSrc structures: `#include <studio.h>` (`studiohdr_t`, `mstudiomesh_t`, and so on)
- MetaHook/Valve interface factory: `#include <metahook.h>` + `EXPOSE_SINGLE_INTERFACE`
- FreeImage: `#include <FreeImage.h>`; linker item `$(FreeImageLibraryFiles)` (from `PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.vcxproj:79/114`)
- ScopeExit: `#include <ScopeExit/ScopeExit.h>` (uses `SCOPE_EXIT` for `FreeImage_*` resource cleanup)

## Notes / Potential Issues (Security and Correctness)
These points are strongly related to defensive parsing and should be prioritized if robustness is improved further:
- `CheckStudioModel_Hitboxes` calculates `pbbox_end` but does not verify whether it is out of bounds (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:406`); OOB remains theoretically possible.
- `CheckStudioModel_BoneControllers`:
  - `pbonecontroller_end = base + studiohdr->numbones` should very likely be `+ studiohdr->numbonecontrollers` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:903`).
- `CheckStudioModel_Bone`: the upper bound for `pbone->bonecontroller[j]` is `studiohdr->numbones`; semantically, it should more likely be checked against `numbonecontrollers` (`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:794`).
- Multiple bounds checks use `> buf+bufSize` rather than `>=`, and some compare only an “element pointer” without covering the element size; overall, this attempts to prevent crashes but does not guarantee the absence of strict vulnerabilities.
- `CheckStudioModel_TextureData` has no explicit overflow protection for `palsize = width*height`; extreme malformed values may bypass some checks.
- `CheckStudioModel_SeqDescAnim` uses `(panimvalue + 255)` as an approximate upper bound, which is a heuristic guardrail rather than strict parsing.

## Applicable Scenarios
- Downloaders/model managers: filter `mdl/bmp` assets from network/third-party sources before persisting them.
- Toolchains (such as `toolsrc/studiocheck`): batch-scan asset packages.

## Extension Suggestions (If Further Improvement Is Needed)
- Add strict `base/end` bounds validation for every block (especially hitboxes and bonecontrollers).
- Convert all size calculations to `size_t` and add `mul_overflow`-style protection.
- Consolidate the “upper-bound constant limits” from HLSDK constants and project-specific constants (for example, `numtextures` currently uses `MAXSTUDIOSKINS` as its upper bound).
