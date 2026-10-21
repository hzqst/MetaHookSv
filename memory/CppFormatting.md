---
title: CppFormatting
type: guide
permalink: metahooksv/cpp-formatting
tags:
- build
- tooling
- conventions
---

# C/C++ formatting

Developer-only formatting and validation notes for the MetaHookSv aggregator and its
components. `AGENTS.md` keeps the binding rules and links here for the setup details.
Each component repository keeps the same note under its own permalink prefix
(`<component>/cpp-formatting`, file `memory/CppFormatting.md`).

## Toolchain and style

Formatting uses [MetaHookSv/FormatValidation](https://github.com/MetaHookSv/FormatValidation)
and clang-format **23.1.3**, with the DiligentCore style (4 spaces, preserved include
order). Install the formatter for the Python interpreter used by CMake:

```sh
python -m pip install clang-format==23.1.3
cmake -S . -B build/format "-DFORMAT_VALIDATION_ONLY=ON"
cmake --build build/format --target format-check
cmake --build build/format --target format
```

The format-only configuration needs CMake 3.21+, Git, Python 3.9+ (CI uses 3.12),
and a build generator; `-G Ninja` works without Visual Studio. It prepares no native
SDK or game dependencies. Formatting targets are explicit and are not part of a
normal DLL build. With a Visual Studio generator, add `--config Debug` or
`--config Release` when building a formatting target.

## Source path resolution

The aggregate provides `FORMAT_VALIDATION_SOURCE_PATH=thirdparty/FormatValidation`.
Standalone components accept that CMake variable or its environment counterpart;
if empty, FetchContent downloads the fixed tooling commit. Quote relative paths,
for example `"-DFORMAT_VALIDATION_SOURCE_PATH=../../thirdparty/FormatValidation"`.
Configuration generates the ignored root `.clang-format` for editors; change the
shared style rather than that generated copy. An optional
`FORMAT_VALIDATION_CLANG_FORMAT_EXECUTABLE` selects an explicit formatter, whose
version must still match the pin.

## Checked scope

Checks cover owned C/C++ files in `src/`, `include/`, and `tests/`, including
non-ignored new files. Repository-relative exclusions live in `.clang-format-ignore`.
Third-party sources and build artifacts are excluded. The `clang-format` workflow
checks the full scope on pushes, pull requests, and manual runs.

## Known constraints

- The same targets exist in every component; standalone checkouts rely on the
  `FORMAT_VALIDATION_SOURCE_PATH` fallback instead of the aggregator submodule.
- Keep MSVC inline assembly inside explicit blocks; formatting unbraced assembly can
  absorb the following C++ statements.
- Do not move the retained FormatValidation module tag; standalone bootstraps pin it.
