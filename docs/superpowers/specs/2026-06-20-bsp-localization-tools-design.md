# BSPLocalizationTools Design

## Goal

Initialize `toolsrc/BSPLocalizationTools` as a .NET command-line tool that extracts `game_text` messages from a GoldSrc BSP entities lump, translates them through an OpenAI-compatible LLM, and writes a localization dictionary CSV next to the input BSP.

## Context

- The repository is primarily C++/MSBuild, but `toolsrc/MetahookInstaller` already uses .NET 8 projects.
- `toolsrc/BSPLocalizationTools` currently contains only a README and test map assets.
- BSP parsing should use `LogicAndTrick/sledge-formats`, specifically the `Sledge.Formats.Bsp` NuGet package.
- The reference CSV `toolsrc/BSPLocalizationTools/tests/maps/pizza_ya_san1_dictionary_schinese.csv` uses the header:
  `Title,Translation,Color,Duration,Speaker,Nextsubtitle,Delaytonextsubtitle,Style`
- The reference schinese CSV is encoded as GBK/CP936 without BOM.
- LLM behavior should mimic the OpenAI-compatible flow in `D:\CS2_VibeSignatures\ida_llm_utils.py`, including optional `fake_as=codex` support for the Responses API transport.

## CLI

The tool accepts:

- `-bsp=<path>`: required BSP file path.
- `-outlang=<lang>`: optional target language, default `schinese`.
- `-promptfile=<path>`: optional UTF-8 prompt file. If omitted, use the built-in prompt.
- `-llm_model=<model>`: optional model name, also read from `BSPL10N_LLM_MODEL`.
- `-llm_apikey=<key>`: optional API key, also read from `BSPL10N_LLM_APIKEY`.
- `-llm_baseurl=<url>`: optional OpenAI-compatible base URL, also read from `BSPL10N_LLM_BASEURL`.
- `-llm_temperature=<value>`: optional temperature, also read from `BSPL10N_LLM_TEMPERATURE`.
- `-llm_effort=<effort>`: optional reasoning effort, default `medium`, also read from `BSPL10N_LLM_EFFORT`.
- `-llm_fake_as=<value>`: optional compatibility mode, only `codex` is supported, also read from `BSPL10N_LLM_FAKE_AS`.

Output path is always the BSP directory plus `<map>_dictionary_<outlang>.csv`, for example:

```text
toolsrc\BSPLocalizationTools\tests\maps\pizza_ya_san1_dictionary_schinese.csv
```

## Architecture

The project will be a .NET 8 console app with focused internal classes:

- `Program`: entry point and exit-code handling.
- `CommandLineOptions`: parses `-key=value` arguments and environment fallbacks.
- `BspGameTextExtractor`: loads the BSP through `Sledge.Formats.Bsp` and extracts entities where `classname == "game_text"` and `message` is non-empty.
- `TranslationPromptBuilder`: combines the built-in or file prompt with extracted messages and target language instructions.
- `ILLMClient`: abstraction for LLM calls so tests can use a fake translator.
- `OpenAiCompatibleLlmClient`: implements Chat Completions transport and `fake_as=codex` Responses API transport.
- `DictionaryCsvWriter`: writes dictionary rows with target-language encoding and CSV escaping.

The implementation should avoid unrelated integration into `MetaHook.sln` unless later requested. The tool can be built directly with `dotnet build toolsrc\BSPLocalizationTools`.

## Data Flow

1. Parse CLI options and validate the BSP path.
2. Load BSP using `Sledge.Formats.Bsp`.
3. Read the entities lump and parse entity key/value blocks through the library-provided entity representation when available.
4. Extract all `game_text` messages, preserving source order.
5. Deduplicate identical messages for translation, while preserving repeated rows if the BSP contains repeated entities.
6. Build one batch LLM request containing target language, formatting rules, and the source messages.
7. Require the LLM response to be JSON with exact source-to-translation mapping.
8. Write the CSV rows:
   - `Title`: `NETMESSAGE:<source message>`
   - `Translation`: translated message
   - remaining columns empty

Message escape sequences such as `\n` remain textual backslash escapes in both title and translation fields.

## Prompt Contract

The built-in prompt should instruct the LLM to:

- Translate GoldSrc `game_text` messages into the requested target language.
- Preserve line breaks as literal `\n`.
- Preserve punctuation intensity and gameplay urgency.
- Keep proper nouns when appropriate.
- Return only valid JSON.
- Include every input item exactly once, keyed by a stable id or source string.

When `-promptfile` is supplied, the custom prompt replaces the main translation instructions but the tool still appends a strict machine-readable output contract.

## Encoding

`DictionaryCsvWriter` registers code-page providers and maps languages as follows:

- `schinese`: GBK/CP936.
- `tchinese`: Big5/CP950.
- Other languages: UTF-8 without BOM.

Unsupported encodings fail with a clear error message. The CSV should not include a BOM for GBK or Big5.

## Error Handling

The tool exits non-zero and prints a clear error when:

- Required `-bsp` is missing.
- The BSP file does not exist.
- The BSP cannot be parsed.
- No `game_text` messages are found.
- LLM configuration is incomplete for a real translation request.
- The LLM response is empty, invalid JSON, or misses any source message.
- The output CSV cannot be written.

The tool should not silently write partial translation output after an LLM failure.

## Testing

Unit tests should cover:

- CLI parsing and environment fallback.
- Language-to-encoding mapping.
- CSV escaping and target encoding behavior.
- `game_text` extraction from `pizza_ya_san1.bsp`.
- Translation flow with a fake `ILLMClient`.

Integration verification should run:

```powershell
dotnet build toolsrc\BSPLocalizationTools
dotnet test toolsrc\BSPLocalizationTools
```

If real LLM credentials are unavailable, only fake-client tests are required.

## Out Of Scope

- Editing the BSP file.
- Translating non-`game_text` entity fields.
- Adding the tool to `MetaHook.sln`.
- Running real LLM calls in automated tests.
- Supporting a separate output path option.
