# Code Style and Conventions for MetaHookSv

## Naming Conventions

### Variables
- **Global variables**: Prefix with `g_` (e.g., `g_pMetaHookAPI`, `g_bIsSvenCoop`)
- **Pointer variables**: Prefix with `p` (e.g., `pEntity`, `pModel`)
- **Member variables**: Prefix with `m_` (e.g., `m_iWidth`, `m_flHeight`)
- **Static variables**: Prefix with `s_` (e.g., `s_iCounter`)
- **Hungarian notation**: Used for type indication (e.g., `i` for int, `fl` for float, `b` for bool)

### Functions
- **PascalCase** for exported functions (e.g., `HUD_Init`, `V_CalcRefdef`)
- **PascalCase** for class methods (e.g., `StudioDrawPlayer`, `SetupRenderer`)
- **Prefix with module name** for internal functions (e.g., `R_StudioDrawPlayer`, `GL_SetRenderMode`)

### Classes and Structs
- **PascalCase** for class names (e.g., `CGameStudioRenderer`, `CBaseEntity`)
- **Prefix with C** for classes (e.g., `CRenderer`, `CShaderManager`)

### Constants and Macros
- **UPPER_CASE** with underscores (e.g., `STUDIO_NF_FLATSHADE`, `MAX_TEXTURE_SIZE`)

### Files
- **lowercase with underscores** for implementation files (e.g., `gl_rmain.cpp`, `gl_studio.cpp`)
- **lowercase with underscores** for headers (e.g., `gl_local.h`, `gl_shader.h`)

## Code Organization

### Header Files
- Use include guards: `#pragma once`
- Group includes: system headers, third-party, project headers
- Forward declarations when possible

### Source Files
- Include corresponding header first
- Group related functions together
- Use anonymous namespaces for internal helpers

## Comments
- Use `//` for single-line comments
- Use `/* */` for multi-line comments
- Document complex algorithms and non-obvious code
- Add TODO comments for future work: `// TODO: description`

## Formatting
- **Indentation**: Tabs (appears to be the project standard)
- **Braces**: Opening brace on same line for functions and control structures
- **Line length**: No strict limit, but keep reasonable
- **Spacing**: Space after keywords, around operators

## Best Practices
- Check engine type before engine-specific operations using `g_pMetaHookAPI->GetEngineType()`
- Use MetaHook API for hooking and memory operations
- Follow existing plugin structure and patterns
- Leverage PluginLibs for common functionality
- All plugins export standard entry points via `exportfuncs.h`

## Security
- This is a defensive security tool for game modification
- All hooks and patches are for legitimate game enhancement
- No malicious code or exploits should be introduced
