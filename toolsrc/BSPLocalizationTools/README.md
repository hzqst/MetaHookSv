# BSPLocalizationTools

Toolsets for GoldSrc BSP Localization.

## Projects

- `BSPLocalizationToolsCore`: shared localization library.
- `BSPLocalizationToolsCLI`: command line wrapper.
- `BSPLocalizationTools`: Avalonia GUI wrapper.

## CLI Usage

```powershell
dotnet run --project toolsrc\BSPLocalizationTools\src\BSPLocalizationToolsCLI -- `
  "-bsp=toolsrc\BSPLocalizationTools\tests\maps\pizza_ya_san1.bsp" `
  "-outlang=schinese" `
  "-promptfile=path\to\prompt.md" `
  "-llm_model=gpt-5.5" `
  "-llm_apikey=<key>"
```

Output is written next to the BSP as `<map>_dictionary_<outlang>.csv`.

## Arguments

- `-bsp=<path>`: required BSP file.
- `-outlang=<lang>`: optional target language, defaults to `schinese`.
- `-promptfile=<path>`: optional UTF-8 custom translation prompt. When omitted, the tool looks
  next to the BSP for `<map>_prompt_<outlang>.md`, then `<map>_prompt.md`, before falling back
  to the built-in prompt.
- `-llm_model=<model>` or `BSPL10N_LLM_MODEL`.
- `-llm_apikey=<key>` or `BSPL10N_LLM_APIKEY`.
- `-llm_baseurl=<url>` or `BSPL10N_LLM_BASEURL`.
- `-llm_temperature=<value>` or `BSPL10N_LLM_TEMPERATURE`.
- `-llm_effort=<effort>` or `BSPL10N_LLM_EFFORT`, defaults to `medium`.
- `-llm_fake_as=codex` or `BSPL10N_LLM_FAKE_AS=codex`.

The tool loads a `.env` file from the current directory or any parent directory before reading
environment variables. Command line arguments override environment variables, and existing
process environment variables override values from `.env`.

The GUI reads and writes `.env` next to the running tool by default. Prompt configuration is stored
as a prompt file path; prompt text remains in a UTF-8 `.md` file.

Example `.env`:

```env
BSPL10N_LLM_MODEL=gpt-5.5
BSPL10N_LLM_APIKEY=<key>
BSPL10N_LLM_BASEURL=https://api.openai.com/v1
BSPL10N_LLM_EFFORT=medium
BSPL10N_DEFAULT_OUTLANG=schinese
BSPL10N_DEFAULT_PROMPTFILE=path\to\prompt.md
```

## GUI

Run the Avalonia app from `toolsrc\BSPLocalizationTools\src\BSPLocalizationTools`.
The **Translate** tab supports selecting one or more `.bsp` files, starting or canceling
translation, and viewing per-file progress and logs. The **Settings** tab loads and saves LLM and
prompt path settings to `.env`.

## Encodings

CSV output is always written as UTF-8 with BOM.
