# BSPLocalizationTools

Toolsets for GoldSrc BSP Localization.

## Usage

```powershell
dotnet run --project toolsrc\BSPLocalizationTools\src\BSPLocalizationTools -- `
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
- `-promptfile=<path>`: optional UTF-8 custom translation prompt.
- `-llm_model=<model>` or `BSPL10N_LLM_MODEL`.
- `-llm_apikey=<key>` or `BSPL10N_LLM_APIKEY`.
- `-llm_baseurl=<url>` or `BSPL10N_LLM_BASEURL`.
- `-llm_temperature=<value>` or `BSPL10N_LLM_TEMPERATURE`.
- `-llm_effort=<effort>` or `BSPL10N_LLM_EFFORT`, defaults to `medium`.
- `-llm_fake_as=codex` or `BSPL10N_LLM_FAKE_AS=codex`.

## Encodings

- `schinese`: GBK/CP936.
- `tchinese`: Big5/CP950.
- Other languages: UTF-8 without BOM.
