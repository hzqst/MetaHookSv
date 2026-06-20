# BSPLocalizationTools Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a .NET 8 CLI under `toolsrc/BSPLocalizationTools` that extracts GoldSrc BSP `game_text` messages, translates them with an OpenAI-compatible LLM, and writes `<map>_dictionary_<outlang>.csv`.

**Architecture:** Use a small solution with a console app and xUnit tests. Keep BSP parsing, CLI options, prompt/LLM handling, response parsing, and CSV writing in separate focused classes. The app writes only the CSV output and does not edit BSP files.

**Tech Stack:** .NET 8, C# nullable enabled, `Sledge.Formats.Bsp` 1.0.17, `System.Text.Encoding.CodePages`, `System.Text.Json`, xUnit.

---

## Scope And File Structure

Create:

- `toolsrc/BSPLocalizationTools/BSPLocalizationTools.sln`: solution for app and tests.
- `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/BSPLocalizationTools.csproj`: console app project.
- `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/Program.cs`: top-level entry point and exit codes.
- `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/AppRunner.cs`: orchestration from options to output CSV.
- `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/CommandLineOptions.cs`: `-key=value` parsing and env fallback.
- `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/GameTextEntry.cs`: extracted message record.
- `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/BspGameTextExtractor.cs`: `Sledge.Formats.Bsp` integration.
- `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/DictionaryCsvWriter.cs`: CSV escaping and encoded output.
- `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/LanguageEncoding.cs`: target language to encoding mapping.
- `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/LLMMessage.cs`: chat message DTO.
- `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/LLMOptions.cs`: LLM config DTO.
- `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/ILLMClient.cs`: LLM abstraction.
- `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/OpenAICompatibleLLMClient.cs`: Chat Completions plus `fake_as=codex` Responses transport.
- `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/TranslationPromptBuilder.cs`: built-in/custom prompt assembly.
- `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/TranslationResponseParser.cs`: strict JSON response parsing.
- `toolsrc/BSPLocalizationTools/tests/BSPLocalizationTools.Tests/BSPLocalizationTools.Tests.csproj`: test project.
- `toolsrc/BSPLocalizationTools/tests/BSPLocalizationTools.Tests/GlobalUsings.cs`: test-only global imports.
- Test files under `toolsrc/BSPLocalizationTools/tests/BSPLocalizationTools.Tests/`.

Modify:

- `toolsrc/BSPLocalizationTools/README.md`: usage and env variables.

Do not modify:

- `MetaHook.sln`
- BSP fixtures under `toolsrc/BSPLocalizationTools/tests/maps/`
- root build/CI config

Do not commit unless the user explicitly asks. The repository rules override the generic "frequent commits" advice.

---

### Task 1: Scaffold Solution And Projects

**Files:**
- Create: `toolsrc/BSPLocalizationTools/BSPLocalizationTools.sln`
- Create: `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/BSPLocalizationTools.csproj`
- Create: `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/Program.cs`
- Create: `toolsrc/BSPLocalizationTools/tests/BSPLocalizationTools.Tests/BSPLocalizationTools.Tests.csproj`
- Create: `toolsrc/BSPLocalizationTools/tests/BSPLocalizationTools.Tests/GlobalUsings.cs`

- [ ] **Step 1: Create the app and test directories**

Run:

```powershell
New-Item -ItemType Directory -Force -Path `
  'toolsrc\BSPLocalizationTools\src\BSPLocalizationTools', `
  'toolsrc\BSPLocalizationTools\tests\BSPLocalizationTools.Tests' | Out-Null
```

Expected: command exits 0 and creates both directories.

- [ ] **Step 2: Create `BSPLocalizationTools.csproj`**

Create `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/BSPLocalizationTools.csproj`:

```xml
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <OutputType>Exe</OutputType>
    <TargetFramework>net8.0</TargetFramework>
    <ImplicitUsings>enable</ImplicitUsings>
    <Nullable>enable</Nullable>
    <LangVersion>latest</LangVersion>
  </PropertyGroup>

  <ItemGroup>
    <PackageReference Include="Sledge.Formats.Bsp" Version="1.0.17" />
    <PackageReference Include="System.Text.Encoding.CodePages" Version="8.0.0" />
  </ItemGroup>
</Project>
```

- [ ] **Step 3: Create `BSPLocalizationTools.Tests.csproj`**

Create `toolsrc/BSPLocalizationTools/tests/BSPLocalizationTools.Tests/BSPLocalizationTools.Tests.csproj`:

```xml
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <TargetFramework>net8.0</TargetFramework>
    <ImplicitUsings>enable</ImplicitUsings>
    <Nullable>enable</Nullable>
    <LangVersion>latest</LangVersion>
    <IsPackable>false</IsPackable>
  </PropertyGroup>

  <ItemGroup>
    <PackageReference Include="Microsoft.NET.Test.Sdk" Version="17.12.0" />
    <PackageReference Include="xunit" Version="2.9.2" />
    <PackageReference Include="xunit.runner.visualstudio" Version="2.8.2">
      <IncludeAssets>runtime; build; native; contentfiles; analyzers; buildtransitive</IncludeAssets>
      <PrivateAssets>all</PrivateAssets>
    </PackageReference>
  </ItemGroup>

  <ItemGroup>
    <ProjectReference Include="..\..\src\BSPLocalizationTools\BSPLocalizationTools.csproj" />
  </ItemGroup>

  <ItemGroup>
    <None Include="..\maps\**\*">
      <Link>maps\%(RecursiveDir)%(Filename)%(Extension)</Link>
      <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>
    </None>
  </ItemGroup>
</Project>
```

- [ ] **Step 4: Create a temporary app entry point**

Create `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/Program.cs`:

```csharp
Console.WriteLine("BSPLocalizationTools is not wired yet.");
return 1;
```

This keeps the executable project buildable while the implementation grows behind tests.

- [ ] **Step 5: Create test global usings**

Create `toolsrc/BSPLocalizationTools/tests/BSPLocalizationTools.Tests/GlobalUsings.cs`:

```csharp
global using BSPLocalizationTools;
global using Xunit;
```

- [ ] **Step 6: Create the solution**

Run:

```powershell
dotnet new sln --name BSPLocalizationTools --output toolsrc\BSPLocalizationTools
dotnet sln toolsrc\BSPLocalizationTools\BSPLocalizationTools.sln add `
  toolsrc\BSPLocalizationTools\src\BSPLocalizationTools\BSPLocalizationTools.csproj `
  toolsrc\BSPLocalizationTools\tests\BSPLocalizationTools.Tests\BSPLocalizationTools.Tests.csproj
```

Expected: both projects are added to the solution.

- [ ] **Step 7: Restore**

Run:

```powershell
dotnet restore toolsrc\BSPLocalizationTools
```

Expected: restore exits 0.

---

### Task 2: Command-Line Options

**Files:**
- Create: `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/LLMOptions.cs`
- Create: `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/CommandLineOptions.cs`
- Create: `toolsrc/BSPLocalizationTools/tests/BSPLocalizationTools.Tests/CommandLineOptionsTests.cs`

- [ ] **Step 1: Write failing tests**

Create `CommandLineOptionsTests.cs`:

```csharp
namespace BSPLocalizationTools.Tests;

public sealed class CommandLineOptionsTests
{
    [Fact]
    public void ParseUsesDefaultsAndRequiredBsp()
    {
        var options = CommandLineOptions.Parse(
            ["-bsp=C:\\maps\\pizza.bsp"],
            _ => null);

        Assert.Equal("C:\\maps\\pizza.bsp", options.BspPath);
        Assert.Equal("schinese", options.OutLang);
        Assert.Null(options.PromptFilePath);
    }

    [Fact]
    public void ParseReadsLLMEnvironmentFallbacks()
    {
        var env = new Dictionary<string, string?>
        {
            ["BSPL10N_LLM_MODEL"] = "gpt-test",
            ["BSPL10N_LLM_APIKEY"] = "key",
            ["BSPL10N_LLM_BASEURL"] = "https://example.test/v1",
            ["BSPL10N_LLM_TEMPERATURE"] = "0.2",
            ["BSPL10N_LLM_EFFORT"] = "high",
            ["BSPL10N_LLM_FAKE_AS"] = "codex",
        };

        var options = CommandLineOptions.Parse(
            ["-bsp=map.bsp", "-outlang=tchinese", "-promptfile=prompt.md"],
            key => env.TryGetValue(key, out var value) ? value : null);

        Assert.Equal("tchinese", options.OutLang);
        Assert.Equal("prompt.md", options.PromptFilePath);
        Assert.Equal("gpt-test", options.LLM.Model);
        Assert.Equal("key", options.LLM.ApiKey);
        Assert.Equal("https://example.test/v1", options.LLM.BaseUrl);
        Assert.Equal(0.2, options.LLM.Temperature);
        Assert.Equal("high", options.LLM.Effort);
        Assert.Equal("codex", options.LLM.FakeAs);
    }

    [Fact]
    public void ParseRejectsUnknownArgument()
    {
        var ex = Assert.Throws<ArgumentException>(() =>
            CommandLineOptions.Parse(["-bsp=map.bsp", "-wat=no"], _ => null));

        Assert.Contains("Unknown argument", ex.Message);
    }
}
```

- [ ] **Step 2: Run tests and verify failure**

Run:

```powershell
dotnet test toolsrc\BSPLocalizationTools --filter CommandLineOptionsTests
```

Expected: FAIL because `CommandLineOptions` does not exist.

- [ ] **Step 3: Implement options DTOs**

Create `LLMOptions.cs`:

```csharp
namespace BSPLocalizationTools;

public sealed record LLMOptions(
    string? Model,
    string? ApiKey,
    string? BaseUrl,
    double? Temperature,
    string Effort,
    string? FakeAs);
```

Create `CommandLineOptions.cs`:

```csharp
using System.Globalization;

namespace BSPLocalizationTools;

public sealed record CommandLineOptions(
    string BspPath,
    string OutLang,
    string? PromptFilePath,
    LLMOptions LLM)
{
    private static readonly HashSet<string> KnownKeys = new(StringComparer.OrdinalIgnoreCase)
    {
        "bsp", "outlang", "promptfile", "llm_model", "llm_apikey",
        "llm_baseurl", "llm_temperature", "llm_effort", "llm_fake_as",
    };

    public static CommandLineOptions Parse(string[] args, Func<string, string?> getEnv)
    {
        var values = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        foreach (var arg in args)
        {
            if (!arg.StartsWith("-", StringComparison.Ordinal) || !arg.Contains('='))
            {
                throw new ArgumentException($"Invalid argument '{arg}'. Expected -key=value.");
            }

            var trimmed = arg.TrimStart('-');
            var separator = trimmed.IndexOf('=');
            var key = trimmed[..separator];
            var value = trimmed[(separator + 1)..];
            if (!KnownKeys.Contains(key))
            {
                throw new ArgumentException($"Unknown argument '-{key}'.");
            }

            values[key] = value;
        }

        var bspPath = GetRequired(values, "bsp");
        var outLang = GetOptional(values, "outlang") ?? "schinese";
        var promptFile = GetOptional(values, "promptfile");

        return new CommandLineOptions(
            bspPath,
            outLang,
            promptFile,
            new LLMOptions(
                GetOptional(values, "llm_model") ?? getEnv("BSPL10N_LLM_MODEL"),
                GetOptional(values, "llm_apikey") ?? getEnv("BSPL10N_LLM_APIKEY"),
                GetOptional(values, "llm_baseurl") ?? getEnv("BSPL10N_LLM_BASEURL"),
                ParseNullableDouble(GetOptional(values, "llm_temperature") ?? getEnv("BSPL10N_LLM_TEMPERATURE")),
                GetOptional(values, "llm_effort") ?? getEnv("BSPL10N_LLM_EFFORT") ?? "medium",
                NormalizeFakeAs(GetOptional(values, "llm_fake_as") ?? getEnv("BSPL10N_LLM_FAKE_AS"))));
    }

    private static string GetRequired(Dictionary<string, string> values, string key)
    {
        var value = GetOptional(values, key);
        if (string.IsNullOrWhiteSpace(value))
        {
            throw new ArgumentException($"Required argument '-{key}=...' is missing.");
        }

        return value;
    }

    private static string? GetOptional(Dictionary<string, string> values, string key)
    {
        return values.TryGetValue(key, out var value) && !string.IsNullOrWhiteSpace(value)
            ? value.Trim()
            : null;
    }

    private static double? ParseNullableDouble(string? value)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            return null;
        }

        if (!double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out var parsed))
        {
            throw new ArgumentException("LLM temperature must be a number.");
        }

        return parsed;
    }

    private static string? NormalizeFakeAs(string? value)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            return null;
        }

        var normalized = value.Trim().ToLowerInvariant();
        if (normalized != "codex")
        {
            throw new ArgumentException("Only '-llm_fake_as=codex' is supported.");
        }

        return normalized;
    }
}
```

- [ ] **Step 4: Run tests**

Run:

```powershell
dotnet test toolsrc\BSPLocalizationTools --filter CommandLineOptionsTests
```

Expected: PASS.

---

### Task 3: CSV Encoding And Writer

**Files:**
- Create: `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/LanguageEncoding.cs`
- Create: `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/DictionaryCsvWriter.cs`
- Create: `toolsrc/BSPLocalizationTools/tests/BSPLocalizationTools.Tests/DictionaryCsvWriterTests.cs`

- [ ] **Step 1: Write failing tests**

Create `DictionaryCsvWriterTests.cs`:

```csharp
using System.Text;

namespace BSPLocalizationTools.Tests;

public sealed class DictionaryCsvWriterTests
{
    [Fact]
    public void SchineseUsesGbkEncodingWithoutBom()
    {
        using var temp = new TempDirectory();
        var path = Path.Combine(temp.Path, "map_dictionary_schinese.csv");
        var writer = new DictionaryCsvWriter();

        writer.Write(path, "schinese", [new DictionaryRow("NETMESSAGE:a", "出大事儿了！！")]);

        var bytes = File.ReadAllBytes(path);
        Assert.False(bytes.Length >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF);
        var text = Encoding.GetEncoding(936).GetString(bytes);
        Assert.Contains("Title,Translation,Color,Duration,Speaker,Nextsubtitle,Delaytonextsubtitle,Style", text);
        Assert.Contains("NETMESSAGE:a,出大事儿了！！", text);
    }

    [Fact]
    public void TchineseUsesBig5Encoding()
    {
        using var temp = new TempDirectory();
        var path = Path.Combine(temp.Path, "map_dictionary_tchinese.csv");
        var writer = new DictionaryCsvWriter();

        writer.Write(path, "tchinese", [new DictionaryRow("NETMESSAGE:a", "繁體中文")]);

        var text = Encoding.GetEncoding(950).GetString(File.ReadAllBytes(path));
        Assert.Contains("繁體中文", text);
    }

    [Fact]
    public void CsvEscapesCommaQuoteAndNewline()
    {
        using var temp = new TempDirectory();
        var path = Path.Combine(temp.Path, "map_dictionary_schinese.csv");
        var writer = new DictionaryCsvWriter();

        writer.Write(path, "schinese", [new DictionaryRow("NETMESSAGE:a,b", "say \"hello\"\nnow")]);

        var text = Encoding.GetEncoding(936).GetString(File.ReadAllBytes(path));
        Assert.Contains("\"NETMESSAGE:a,b\",\"say \"\"hello\"\"\nnow\"", text);
    }
}
```

Also create `TempDirectory.cs` in the test project:

```csharp
namespace BSPLocalizationTools.Tests;

public sealed class TempDirectory : IDisposable
{
    public string Path { get; } = System.IO.Path.Combine(
        System.IO.Path.GetTempPath(),
        "BSPLocalizationTools.Tests",
        Guid.NewGuid().ToString("N"));

    public TempDirectory()
    {
        Directory.CreateDirectory(Path);
    }

    public void Dispose()
    {
        if (Directory.Exists(Path))
        {
            Directory.Delete(Path, recursive: true);
        }
    }
}
```

- [ ] **Step 2: Run tests and verify failure**

Run:

```powershell
dotnet test toolsrc\BSPLocalizationTools --filter DictionaryCsvWriterTests
```

Expected: FAIL because writer classes do not exist.

- [ ] **Step 3: Implement writer**

Create `LanguageEncoding.cs`:

```csharp
using System.Text;

namespace BSPLocalizationTools;

public static class LanguageEncoding
{
    public static Encoding Resolve(string language)
    {
        Encoding.RegisterProvider(CodePagesEncodingProvider.Instance);
        return language.Trim().ToLowerInvariant() switch
        {
            "schinese" => Encoding.GetEncoding(936),
            "tchinese" => Encoding.GetEncoding(950),
            _ => new UTF8Encoding(encoderShouldEmitUTF8Identifier: false),
        };
    }
}
```

Create `DictionaryCsvWriter.cs`:

```csharp
using System.Text;

namespace BSPLocalizationTools;

public sealed record DictionaryRow(string Title, string Translation);

public sealed class DictionaryCsvWriter
{
    private const string Header = "Title,Translation,Color,Duration,Speaker,Nextsubtitle,Delaytonextsubtitle,Style";

    public void Write(string outputPath, string language, IReadOnlyList<DictionaryRow> rows)
    {
        var encoding = LanguageEncoding.Resolve(language);
        var directory = Path.GetDirectoryName(outputPath);
        if (!string.IsNullOrEmpty(directory))
        {
            Directory.CreateDirectory(directory);
        }

        using var writer = new StreamWriter(outputPath, append: false, encoding);
        writer.WriteLine(Header);
        foreach (var row in rows)
        {
            writer.Write(Escape(row.Title));
            writer.Write(',');
            writer.Write(Escape(row.Translation));
            writer.WriteLine(",,,,,,");
        }
    }

    private static string Escape(string value)
    {
        if (value.IndexOfAny([',', '"', '\r', '\n']) < 0)
        {
            return value;
        }

        return "\"" + value.Replace("\"", "\"\"", StringComparison.Ordinal) + "\"";
    }
}
```

- [ ] **Step 4: Run tests**

Run:

```powershell
dotnet test toolsrc\BSPLocalizationTools --filter DictionaryCsvWriterTests
```

Expected: PASS.

---

### Task 4: BSP `game_text` Extraction

**Files:**
- Create: `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/GameTextEntry.cs`
- Create: `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/BspGameTextExtractor.cs`
- Create: `toolsrc/BSPLocalizationTools/tests/BSPLocalizationTools.Tests/BspGameTextExtractorTests.cs`

- [ ] **Step 1: Write failing tests**

Create `BspGameTextExtractorTests.cs`:

```csharp
namespace BSPLocalizationTools.Tests;

public sealed class BspGameTextExtractorTests
{
    [Fact]
    public void ExtractsGameTextMessagesFromPizzaYaSanMap()
    {
        var extractor = new BspGameTextExtractor();

        var entries = extractor.Extract(FindMap("pizza_ya_san1.bsp"));

        Assert.Equal(74, entries.Count);
        Assert.Contains(entries, entry =>
            entry.Message == "kinnkyuu jitai da!!\\n(We got a situation!!)");
        Assert.Contains(entries, entry =>
            entry.Message == "pizza wo nerau bakemono ga kuru zo!!\\n(The monsters tracking down pizza are incoming!!)");
    }

    private static string FindMap(string name)
    {
        var directory = new DirectoryInfo(AppContext.BaseDirectory);
        while (directory is not null)
        {
            var candidate = Path.Combine(directory.FullName, "maps", name);
            if (File.Exists(candidate))
            {
                return candidate;
            }

            directory = directory.Parent;
        }

        throw new FileNotFoundException($"Could not find test map '{name}'.");
    }
}
```

- [ ] **Step 2: Run tests and verify failure**

Run:

```powershell
dotnet test toolsrc\BSPLocalizationTools --filter BspGameTextExtractorTests
```

Expected: FAIL because `BspGameTextExtractor` does not exist.

- [ ] **Step 3: Implement extractor**

Create `GameTextEntry.cs`:

```csharp
namespace BSPLocalizationTools;

public sealed record GameTextEntry(int Index, string Message);

public interface IGameTextExtractor
{
    IReadOnlyList<GameTextEntry> Extract(string bspPath);
}
```

Create `BspGameTextExtractor.cs`:

```csharp
using Sledge.Formats.Bsp;
using Sledge.Formats.Bsp.Lumps;

namespace BSPLocalizationTools;

public sealed class BspGameTextExtractor : IGameTextExtractor
{
    public IReadOnlyList<GameTextEntry> Extract(string bspPath)
    {
        using var stream = File.OpenRead(bspPath);
        var bsp = new BspFile(stream);
        var entities = bsp.GetLump<Entities>()
            ?? throw new InvalidOperationException("BSP does not contain an entities lump.");

        var results = new List<GameTextEntry>();
        foreach (var entity in entities)
        {
            if (!string.Equals(entity.ClassName, "game_text", StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            var message = entity.Get("message", "");
            if (string.IsNullOrWhiteSpace(message))
            {
                continue;
            }

            results.Add(new GameTextEntry(results.Count, message));
        }

        return results;
    }
}
```

- [ ] **Step 4: Run tests**

Run:

```powershell
dotnet test toolsrc\BSPLocalizationTools --filter BspGameTextExtractorTests
```

Expected: PASS.

---

### Task 5: Prompt Builder And Translation JSON Parser

**Files:**
- Create: `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/LLMMessage.cs`
- Create: `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/TranslationPromptBuilder.cs`
- Create: `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/TranslationResponseParser.cs`
- Create: `toolsrc/BSPLocalizationTools/tests/BSPLocalizationTools.Tests/TranslationPromptBuilderTests.cs`
- Create: `toolsrc/BSPLocalizationTools/tests/BSPLocalizationTools.Tests/TranslationResponseParserTests.cs`

- [ ] **Step 1: Write failing tests**

Create `TranslationPromptBuilderTests.cs`:

```csharp
namespace BSPLocalizationTools.Tests;

public sealed class TranslationPromptBuilderTests
{
    [Fact]
    public void BuildIncludesCustomPromptLanguageAndInputs()
    {
        var messages = TranslationPromptBuilder.Build(
            "schinese",
            "Translate with Sven Co-op tone.",
            ["hello\\nworld"]);

        Assert.Equal("system", messages[0].Role);
        Assert.Equal("user", messages[1].Role);
        Assert.Contains("Translate with Sven Co-op tone.", messages[0].Content);
        Assert.Contains("\"target_language\":\"schinese\"", messages[1].Content);
        Assert.Contains("\"id\":0", messages[1].Content);
        Assert.Contains("hello\\\\nworld", messages[1].Content);
    }
}
```

Create `TranslationResponseParserTests.cs`:

```csharp
namespace BSPLocalizationTools.Tests;

public sealed class TranslationResponseParserTests
{
    [Fact]
    public void ParseReadsTranslationsById()
    {
        var parsed = TranslationResponseParser.Parse(
            """
            {"translations":[{"id":0,"translation":"你好"},{"id":1,"translation":"再见"}]}
            """,
            expectedCount: 2);

        Assert.Equal("你好", parsed[0]);
        Assert.Equal("再见", parsed[1]);
    }

    [Fact]
    public void ParseRejectsMissingTranslation()
    {
        var ex = Assert.Throws<InvalidOperationException>(() =>
            TranslationResponseParser.Parse("""{"translations":[{"id":0,"translation":"你好"}]}""", 2));

        Assert.Contains("missing id 1", ex.Message);
    }
}
```

- [ ] **Step 2: Run tests and verify failure**

Run:

```powershell
dotnet test toolsrc\BSPLocalizationTools --filter "TranslationPromptBuilderTests|TranslationResponseParserTests"
```

Expected: FAIL because prompt/parser classes do not exist.

- [ ] **Step 3: Implement prompt and parser**

Create `LLMMessage.cs`:

```csharp
namespace BSPLocalizationTools;

public sealed record LLMMessage(string Role, string Content);
```

Create `TranslationPromptBuilder.cs`:

```csharp
using System.Text.Json;

namespace BSPLocalizationTools;

public static class TranslationPromptBuilder
{
    public const string BuiltInPrompt = """
        Translate GoldSrc/Sven Co-op game_text messages into the requested target language.
        Preserve literal \n sequences, punctuation intensity, urgency, and proper nouns when appropriate.
        Return natural in-game text, not explanatory notes.
        """;

    public static IReadOnlyList<LLMMessage> Build(
        string targetLanguage,
        string? customPrompt,
        IReadOnlyList<string> sourceMessages)
    {
        var instructions = string.IsNullOrWhiteSpace(customPrompt)
            ? BuiltInPrompt
            : customPrompt.Trim();
        var payload = new
        {
            target_language = targetLanguage,
            output_contract = new
            {
                format = "json",
                schema = """{"translations":[{"id":0,"translation":"..."}]}""",
                rules = new[]
                {
                    "Return only valid JSON.",
                    "Include every input id exactly once.",
                    "Keep literal \\n as backslash-n text.",
                },
            },
            inputs = sourceMessages.Select((text, id) => new { id, text }).ToArray(),
        };

        return
        [
            new LLMMessage("system", instructions),
            new LLMMessage("user", JsonSerializer.Serialize(payload)),
        ];
    }
}
```

Create `TranslationResponseParser.cs`:

```csharp
using System.Text.Json;

namespace BSPLocalizationTools;

public static class TranslationResponseParser
{
    public static IReadOnlyDictionary<int, string> Parse(string responseText, int expectedCount)
    {
        var json = StripCodeFence(responseText);
        using var document = JsonDocument.Parse(json);
        if (!document.RootElement.TryGetProperty("translations", out var translations) ||
            translations.ValueKind != JsonValueKind.Array)
        {
            throw new InvalidOperationException("LLM response must contain a translations array.");
        }

        var result = new Dictionary<int, string>();
        foreach (var item in translations.EnumerateArray())
        {
            var id = item.GetProperty("id").GetInt32();
            var translation = item.GetProperty("translation").GetString();
            if (string.IsNullOrWhiteSpace(translation))
            {
                throw new InvalidOperationException($"LLM response translation for id {id} is empty.");
            }

            result[id] = translation;
        }

        for (var i = 0; i < expectedCount; i++)
        {
            if (!result.ContainsKey(i))
            {
                throw new InvalidOperationException($"LLM response missing id {i}.");
            }
        }

        return result;
    }

    private static string StripCodeFence(string text)
    {
        var trimmed = text.Trim();
        if (!trimmed.StartsWith("```", StringComparison.Ordinal))
        {
            return trimmed;
        }

        var firstLineEnd = trimmed.IndexOf('\n');
        var lastFence = trimmed.LastIndexOf("```", StringComparison.Ordinal);
        if (firstLineEnd < 0 || lastFence <= firstLineEnd)
        {
            return trimmed;
        }

        return trimmed[(firstLineEnd + 1)..lastFence].Trim();
    }
}
```

- [ ] **Step 4: Run tests**

Run:

```powershell
dotnet test toolsrc\BSPLocalizationTools --filter "TranslationPromptBuilderTests|TranslationResponseParserTests"
```

Expected: PASS.

---

### Task 6: OpenAI-Compatible LLM Client

**Files:**
- Create: `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/ILLMClient.cs`
- Create: `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/OpenAICompatibleLLMClient.cs`
- Create: `toolsrc/BSPLocalizationTools/tests/BSPLocalizationTools.Tests/OpenAICompatibleLLMClientTests.cs`

- [ ] **Step 1: Write failing tests with a fake handler**

Create `OpenAICompatibleLLMClientTests.cs`:

```csharp
using System.Net;
using System.Text;

namespace BSPLocalizationTools.Tests;

public sealed class OpenAICompatibleLLMClientTests
{
    [Fact]
    public async Task ChatCompletionsReturnsFirstMessageText()
    {
        var handler = new FakeHttpHandler(_ => new HttpResponseMessage(HttpStatusCode.OK)
        {
            Content = new StringContent(
                """{"choices":[{"message":{"content":"{\"translations\":[{\"id\":0,\"translation\":\"你好\"}]}"}}]}""",
                Encoding.UTF8,
                "application/json"),
        });
        var client = new OpenAICompatibleLLMClient(new HttpClient(handler));

        var result = await client.CompleteTextAsync(
            [new LLMMessage("user", "hello")],
            new LLMOptions("gpt-test", "key", "https://example.test/v1", null, "medium", null),
            CancellationToken.None);

        Assert.Contains("translations", result);
        Assert.Equal("https://example.test/v1/chat/completions", handler.RequestUri!.ToString());
    }

    [Fact]
    public async Task CodexResponsesReadsSseDelta()
    {
        var handler = new FakeHttpHandler(_ => new HttpResponseMessage(HttpStatusCode.OK)
        {
            Content = new StringContent(
                "data: {\"type\":\"response.output_text.delta\",\"delta\":\"hello\"}\n\ndata: [DONE]\n\n",
                Encoding.UTF8,
                "text/event-stream"),
        });
        var client = new OpenAICompatibleLLMClient(new HttpClient(handler));

        var result = await client.CompleteTextAsync(
            [new LLMMessage("user", "hello")],
            new LLMOptions("gpt-test", "key", "https://example.test/v1", null, "medium", "codex"),
            CancellationToken.None);

        Assert.Equal("hello", result);
        Assert.Equal("https://example.test/v1/responses", handler.RequestUri!.ToString());
    }

    private sealed class FakeHttpHandler(Func<HttpRequestMessage, HttpResponseMessage> respond) : HttpMessageHandler
    {
        public Uri? RequestUri { get; private set; }

        protected override Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
        {
            RequestUri = request.RequestUri;
            return Task.FromResult(respond(request));
        }
    }
}
```

- [ ] **Step 2: Run tests and verify failure**

Run:

```powershell
dotnet test toolsrc\BSPLocalizationTools --filter OpenAICompatibleLLMClientTests
```

Expected: FAIL because the LLM client does not exist.

- [ ] **Step 3: Implement client**

Create `ILLMClient.cs`:

```csharp
namespace BSPLocalizationTools;

public interface ILLMClient
{
    Task<string> CompleteTextAsync(
        IReadOnlyList<LLMMessage> messages,
        LLMOptions options,
        CancellationToken cancellationToken);
}
```

Create `OpenAICompatibleLLMClient.cs` with these required behaviors:

```csharp
using System.Net.Http.Headers;
using System.Text;
using System.Text.Json;

namespace BSPLocalizationTools;

public sealed class OpenAICompatibleLLMClient(HttpClient httpClient) : ILLMClient
{
    private const string DefaultBaseUrl = "https://api.openai.com/v1";

    public async Task<string> CompleteTextAsync(
        IReadOnlyList<LLMMessage> messages,
        LLMOptions options,
        CancellationToken cancellationToken)
    {
        ValidateOptions(options);
        return string.Equals(options.FakeAs, "codex", StringComparison.OrdinalIgnoreCase)
            ? await CompleteViaResponsesAsync(messages, options, cancellationToken)
            : await CompleteViaChatAsync(messages, options, cancellationToken);
    }

    private static void ValidateOptions(LLMOptions options)
    {
        if (string.IsNullOrWhiteSpace(options.Model))
        {
            throw new InvalidOperationException("LLM model is required. Use -llm_model or BSPL10N_LLM_MODEL.");
        }

        if (string.IsNullOrWhiteSpace(options.ApiKey))
        {
            throw new InvalidOperationException("LLM API key is required. Use -llm_apikey or BSPL10N_LLM_APIKEY.");
        }

        if (string.Equals(options.FakeAs, "codex", StringComparison.OrdinalIgnoreCase) &&
            string.IsNullOrWhiteSpace(options.BaseUrl))
        {
            throw new InvalidOperationException("-llm_baseurl is required when -llm_fake_as=codex.");
        }
    }

    private async Task<string> CompleteViaChatAsync(
        IReadOnlyList<LLMMessage> messages,
        LLMOptions options,
        CancellationToken cancellationToken)
    {
        var body = new Dictionary<string, object?>
        {
            ["model"] = options.Model,
            ["messages"] = messages.Select(m => new { role = m.Role, content = m.Content }).ToArray(),
            ["reasoning_effort"] = options.Effort,
        };
        if (options.Temperature is not null)
        {
            body["temperature"] = options.Temperature;
        }

        using var request = CreateJsonRequest(GetBaseUrl(options) + "/chat/completions", options, body);
        using var response = await httpClient.SendAsync(request, cancellationToken);
        response.EnsureSuccessStatusCode();
        var json = await response.Content.ReadAsStringAsync(cancellationToken);
        using var document = JsonDocument.Parse(json);
        var content = document.RootElement
            .GetProperty("choices")[0]
            .GetProperty("message")
            .GetProperty("content")
            .GetString();
        return string.IsNullOrWhiteSpace(content)
            ? throw new InvalidOperationException("LLM chat response was empty.")
            : content;
    }

    private async Task<string> CompleteViaResponsesAsync(
        IReadOnlyList<LLMMessage> messages,
        LLMOptions options,
        CancellationToken cancellationToken)
    {
        var body = new Dictionary<string, object?>
        {
            ["model"] = options.Model,
            ["input"] = messages.Where(m => m.Role == "user")
                .Select(m => new { role = "user", content = m.Content })
                .ToArray(),
            ["reasoning"] = new { effort = options.Effort },
            ["stream"] = true,
        };
        if (options.Temperature is not null)
        {
            body["temperature"] = options.Temperature;
        }

        using var request = CreateJsonRequest(GetBaseUrl(options) + "/responses", options, body);
        request.Headers.Accept.Clear();
        request.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("text/event-stream"));
        using var response = await httpClient.SendAsync(request, HttpCompletionOption.ResponseHeadersRead, cancellationToken);
        response.EnsureSuccessStatusCode();
        var text = await response.Content.ReadAsStringAsync(cancellationToken);
        var result = ExtractSseText(text);
        return string.IsNullOrWhiteSpace(result)
            ? throw new InvalidOperationException("LLM responses stream was empty.")
            : result;
    }

    private static HttpRequestMessage CreateJsonRequest(string uri, LLMOptions options, object body)
    {
        var request = new HttpRequestMessage(HttpMethod.Post, uri);
        request.Headers.Authorization = new AuthenticationHeaderValue("Bearer", options.ApiKey);
        request.Headers.UserAgent.ParseAdd("BSPLocalizationTools/1.0");
        request.Content = new StringContent(JsonSerializer.Serialize(body), Encoding.UTF8, "application/json");
        return request;
    }

    private static string GetBaseUrl(LLMOptions options)
    {
        return (string.IsNullOrWhiteSpace(options.BaseUrl) ? DefaultBaseUrl : options.BaseUrl.Trim()).TrimEnd('/');
    }

    private static string ExtractSseText(string text)
    {
        var builder = new StringBuilder();
        foreach (var rawLine in text.Split('\n'))
        {
            var line = rawLine.Trim();
            if (!line.StartsWith("data:", StringComparison.Ordinal))
            {
                continue;
            }

            var payload = line[5..].Trim();
            if (payload == "[DONE]")
            {
                break;
            }

            using var document = JsonDocument.Parse(payload);
            var root = document.RootElement;
            if (root.TryGetProperty("type", out var type) &&
                type.GetString() == "response.output_text.delta" &&
                root.TryGetProperty("delta", out var delta))
            {
                builder.Append(delta.GetString());
            }
        }

        return builder.ToString();
    }
}
```

- [ ] **Step 4: Run tests**

Run:

```powershell
dotnet test toolsrc\BSPLocalizationTools --filter OpenAICompatibleLLMClientTests
```

Expected: PASS.

---

### Task 7: App Runner Integration With Fake LLM

**Files:**
- Create: `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/AppRunner.cs`
- Create: `toolsrc/BSPLocalizationTools/tests/BSPLocalizationTools.Tests/AppRunnerTests.cs`

- [ ] **Step 1: Write failing integration test**

Create `AppRunnerTests.cs`:

```csharp
using System.Text;

namespace BSPLocalizationTools.Tests;

public sealed class AppRunnerTests
{
    [Fact]
    public async Task RunWritesDictionaryNextToBsp()
    {
        using var temp = new TempDirectory();
        var bspPath = Path.Combine(temp.Path, "fake_map.bsp");
        File.WriteAllText(bspPath, "placeholder");

        var runner = new AppRunner(
            new FakeExtractor([
                new GameTextEntry(0, "kinnkyuu jitai da!!\\n(We got a situation!!)"),
                new GameTextEntry(1, "kinnkyuu jitai da!!\\n(We got a situation!!)"),
            ]),
            new FakeLLMClient(),
            new DictionaryCsvWriter());

        var output = await runner.RunAsync(
            new CommandLineOptions(
                bspPath,
                "schinese",
                null,
                new LLMOptions("gpt-test", "key", null, null, "medium", null)),
            CancellationToken.None);

        Assert.Equal(Path.Combine(temp.Path, "fake_map_dictionary_schinese.csv"), output);
        var text = Encoding.GetEncoding(936).GetString(File.ReadAllBytes(output));
        Assert.Contains("NETMESSAGE:kinnkyuu jitai da!!\\n(We got a situation!!),緊急事態だ!!\\n（出大事儿了！！）", text);
        Assert.Equal(3, text.Split(Environment.NewLine, StringSplitOptions.RemoveEmptyEntries).Length);
    }

    private sealed class FakeExtractor(IReadOnlyList<GameTextEntry> entries) : IGameTextExtractor
    {
        public IReadOnlyList<GameTextEntry> Extract(string bspPath) => entries;
    }

    private sealed class FakeLLMClient : ILLMClient
    {
        public Task<string> CompleteTextAsync(
            IReadOnlyList<LLMMessage> messages,
            LLMOptions options,
            CancellationToken cancellationToken)
        {
            return Task.FromResult(
                """{"translations":[{"id":0,"translation":"緊急事態だ!!\\n（出大事儿了！！）"}]}""");
        }
    }
}
```

- [ ] **Step 2: Run test and verify failure**

Run:

```powershell
dotnet test toolsrc\BSPLocalizationTools --filter AppRunnerTests
```

Expected: FAIL because `AppRunner` does not exist.

- [ ] **Step 3: Implement `AppRunner`**

Create `AppRunner.cs`:

```csharp
namespace BSPLocalizationTools;

public sealed class AppRunner(
    IGameTextExtractor extractor,
    ILLMClient llmClient,
    DictionaryCsvWriter csvWriter)
{
    public async Task<string> RunAsync(CommandLineOptions options, CancellationToken cancellationToken)
    {
        if (!File.Exists(options.BspPath))
        {
            throw new FileNotFoundException("BSP file was not found.", options.BspPath);
        }

        var prompt = ReadPrompt(options.PromptFilePath);
        var entries = extractor.Extract(options.BspPath);
        if (entries.Count == 0)
        {
            throw new InvalidOperationException("No game_text messages were found.");
        }

        var uniqueMessages = entries.Select(e => e.Message).Distinct(StringComparer.Ordinal).ToArray();
        var llmMessages = TranslationPromptBuilder.Build(options.OutLang, prompt, uniqueMessages);
        var response = await llmClient.CompleteTextAsync(llmMessages, options.LLM, cancellationToken);
        var translations = TranslationResponseParser.Parse(response, uniqueMessages.Length);

        var translatedBySource = uniqueMessages
            .Select((source, id) => new { source, translation = translations[id] })
            .ToDictionary(x => x.source, x => x.translation, StringComparer.Ordinal);
        var rows = entries
            .Select(e => new DictionaryRow("NETMESSAGE:" + e.Message, translatedBySource[e.Message]))
            .ToArray();

        var outputPath = GetOutputPath(options.BspPath, options.OutLang);
        csvWriter.Write(outputPath, options.OutLang, rows);
        return outputPath;
    }

    public static string GetOutputPath(string bspPath, string outLang)
    {
        var directory = Path.GetDirectoryName(Path.GetFullPath(bspPath)) ?? Environment.CurrentDirectory;
        var mapName = Path.GetFileNameWithoutExtension(bspPath);
        return Path.Combine(directory, $"{mapName}_dictionary_{outLang}.csv");
    }

    private static string? ReadPrompt(string? promptFilePath)
    {
        if (string.IsNullOrWhiteSpace(promptFilePath))
        {
            return null;
        }

        if (!File.Exists(promptFilePath))
        {
            throw new FileNotFoundException("Prompt file was not found.", promptFilePath);
        }

        return File.ReadAllText(promptFilePath);
    }
}
```

- [ ] **Step 4: Run test**

Run:

```powershell
dotnet test toolsrc\BSPLocalizationTools --filter AppRunnerTests
```

Expected: PASS.

---

### Task 8: Program Entry Point And README

**Files:**
- Create: `toolsrc/BSPLocalizationTools/src/BSPLocalizationTools/Program.cs`
- Modify: `toolsrc/BSPLocalizationTools/README.md`

- [ ] **Step 1: Implement `Program.cs`**

Create `Program.cs`:

```csharp
using BSPLocalizationTools;

try
{
    var options = CommandLineOptions.Parse(args, Environment.GetEnvironmentVariable);
    using var httpClient = new HttpClient
    {
        Timeout = TimeSpan.FromMinutes(5),
    };
    var runner = new AppRunner(
        new BspGameTextExtractor(),
        new OpenAICompatibleLLMClient(httpClient),
        new DictionaryCsvWriter());

    var output = await runner.RunAsync(options, CancellationToken.None);
    Console.WriteLine($"Wrote dictionary: {output}");
    return 0;
}
catch (Exception ex)
{
    Console.Error.WriteLine("Error: " + ex.Message);
    return 1;
}
```

- [ ] **Step 2: Update README**

Replace `toolsrc/BSPLocalizationTools/README.md` with:

```markdown
# BSPLocalizationTools

Toolsets for GoldSrc BSP localization.

## Usage

```powershell
dotnet run --project toolsrc\BSPLocalizationTools\src\BSPLocalizationTools -- `
  -bsp=toolsrc\BSPLocalizationTools\tests\maps\pizza_ya_san1.bsp `
  -outlang=schinese `
  -promptfile=path\to\prompt.md `
  -llm_model=gpt-5.5 `
  -llm_apikey=<key>
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
```

- [ ] **Step 3: Build**

Run:

```powershell
dotnet build toolsrc\BSPLocalizationTools
```

Expected: build exits 0.

---

### Task 9: Full Verification

**Files:**
- No new files unless previous verification exposes a defect.

- [ ] **Step 1: Run all tests**

Run:

```powershell
dotnet test toolsrc\BSPLocalizationTools
```

Expected: all tests pass. No real LLM calls run.

- [ ] **Step 2: Run CLI missing-config smoke test**

Run:

```powershell
dotnet run --project toolsrc\BSPLocalizationTools\src\BSPLocalizationTools -- `
  -bsp=toolsrc\BSPLocalizationTools\tests\maps\pizza_ya_san1.bsp
```

Expected: exits 1 with a clear LLM configuration error such as `LLM model is required`, and does not create a partial new CSV beyond an already existing fixture.

- [ ] **Step 3: Inspect changed files**

Run:

```powershell
git status --short
git diff -- toolsrc\BSPLocalizationTools docs\superpowers
```

Expected: only planned files changed. Existing untracked scripts outside this task remain untouched.

---

## Self-Review Notes

- Spec coverage: CLI, BSP extraction, `Sledge.Formats.Bsp`, prompt file, LLM parameters, JSON translation response, output naming, GBK/Big5/UTF-8 encodings, error handling, and fake-client tests are covered.
- Placeholder scan: no `TBD`, `TODO`, `implement later`, or unspecified edge handling remains in this plan.
- Type consistency: the approved interface name is `ILLMClient` everywhere; `GameTextEntry`, `DictionaryRow`, `LLMOptions`, and `LLMMessage` are defined before use in later tasks.
- Parallelism: not recommended because tasks build on the same new solution and shared core types.
