using System.Text;

namespace BSPLocalizationTools.Tests;

public sealed class LocalizationRunnerTests
{
    [Fact]
    public async Task RunWritesDictionaryNextToBsp()
    {
        using var temp = new TempDirectory();
        var bspPath = Path.Combine(temp.Path, "fake_map.bsp");
        File.WriteAllText(bspPath, "placeholder");

        var runner = new LocalizationRunner(
            new FakeExtractor([
                new GameTextEntry(0, "kinnkyuu jitai da!!\\n(We got a situation!!)"),
            ]),
            new FakeLLMClient(),
            new DictionaryCsvWriter());

        var output = await runner.RunAsync(
            new LocalizationRequest(
                bspPath,
                "schinese",
                null,
                new LLMOptions("gpt-5.4", "sk-", null, null, "high", null)),
            null,
            CancellationToken.None);

        var outputPath = Assert.IsType<string>(output.OutputPath);
        Assert.Equal(Path.Combine(temp.Path, "fake_map_dictionary_schinese.csv"), outputPath);
        var text = Encoding.UTF8.GetString(File.ReadAllBytes(outputPath));
        Assert.Contains("NETMESSAGE:kinnkyuu jitai da!!\\n(We got a situation!!),緊急事態だ!!\\n（出大事儿了！！）", text);
        Assert.Equal(2, text.Split(Environment.NewLine, StringSplitOptions.RemoveEmptyEntries).Length);
    }

    [Fact]
    public async Task RunWritesDictionaryWithoutLanguageSuffixWhenRequested()
    {
        using var temp = new TempDirectory();
        var bspPath = Path.Combine(temp.Path, "fake_map.bsp");
        File.WriteAllText(bspPath, "placeholder");

        var runner = new LocalizationRunner(
            new FakeExtractor([new GameTextEntry(0, "hello")]),
            new FakeLLMClient("""{"translations":[{"id":0,"translation":"你好"}]}"""),
            new DictionaryCsvWriter());

        var output = await runner.RunAsync(
            new LocalizationRequest(
                bspPath,
                "schinese",
                null,
                new LLMOptions("gpt-5.4", "sk-", null, null, "high", null),
                AppendLanguageToCsvFileName: false),
            null,
            CancellationToken.None);

        Assert.Equal(Path.Combine(temp.Path, "fake_map_dictionary.csv"), output.OutputPath);
        Assert.True(File.Exists(output.OutputPath));
    }

    [Fact]
    public async Task RunSkipsEmptyGameTextWithoutCallingLlmOrWritingCsv()
    {
        using var temp = new TempDirectory();
        var bspPath = Path.Combine(temp.Path, "empty_map.bsp");
        File.WriteAllText(bspPath, "placeholder");
        var llmClient = new FakeLLMClient();
        var runner = new LocalizationRunner(
            new FakeExtractor([]),
            llmClient,
            new DictionaryCsvWriter());

        var output = await runner.RunAsync(
            new LocalizationRequest(
                bspPath,
                "schinese",
                null,
                new LLMOptions("gpt-5.4", "sk-", null, null, "high", null)),
            null,
            CancellationToken.None);

        Assert.True(output.Skipped);
        Assert.Null(output.OutputPath);
        Assert.Null(llmClient.LastMessages);
        Assert.False(File.Exists(Path.Combine(temp.Path, "empty_map_dictionary_schinese.csv")));
    }

    [Fact]
    public async Task RunOmitsRowsWhenTranslationMatchesSourceText()
    {
        using var temp = new TempDirectory();
        var bspPath = Path.Combine(temp.Path, "fake_map.bsp");
        File.WriteAllText(bspPath, "placeholder");

        var runner = new LocalizationRunner(
            new FakeExtractor([
                new GameTextEntry(0, "Weapon Updated!!"),
                new GameTextEntry(1, "hello"),
            ]),
            new FakeLLMClient("""{"translations":[{"id":0,"translation":"Weapon Updated!!"},{"id":1,"translation":"你好"}]}"""),
            new DictionaryCsvWriter());

        var output = await runner.RunAsync(
            new LocalizationRequest(
                bspPath,
                "schinese",
                null,
                new LLMOptions("gpt-5.4", "sk-", null, null, "high", null)),
            null,
            CancellationToken.None);

        var outputPath = Assert.IsType<string>(output.OutputPath);
        var text = Encoding.UTF8.GetString(File.ReadAllBytes(outputPath));
        Assert.DoesNotContain("NETMESSAGE:Weapon Updated!!", text);
        Assert.Contains("NETMESSAGE:hello,你好", text);
        Assert.Equal(2, text.Split(Environment.NewLine, StringSplitOptions.RemoveEmptyEntries).Length);
    }

    [Fact]
    public async Task RunWritesOneDictionaryRowForDuplicateGameTextMessages()
    {
        using var temp = new TempDirectory();
        var bspPath = Path.Combine(temp.Path, "fake_map.bsp");
        File.WriteAllText(bspPath, "placeholder");

        var runner = new LocalizationRunner(
            new FakeExtractor([
                new GameTextEntry(0, "hello"),
                new GameTextEntry(1, "hello"),
                new GameTextEntry(2, "goodbye"),
            ]),
            new FakeLLMClient(
                """{"translations":[{"id":0,"translation":"你好"},{"id":1,"translation":"再见"}]}"""),
            new DictionaryCsvWriter());

        var output = await runner.RunAsync(
            new LocalizationRequest(
                bspPath,
                "schinese",
                null,
                new LLMOptions("gpt-5.4", "sk-", null, null, "high", null)),
            null,
            CancellationToken.None);

        var outputPath = Assert.IsType<string>(output.OutputPath);
        var text = Encoding.UTF8.GetString(File.ReadAllBytes(outputPath));
        Assert.Contains("NETMESSAGE:hello,你好", text);
        Assert.Equal(2, text.Split("NETMESSAGE:hello", StringSplitOptions.None).Length);
        Assert.Contains("NETMESSAGE:goodbye,再见", text);
        Assert.Equal(3, text.Split(Environment.NewLine, StringSplitOptions.RemoveEmptyEntries).Length);
    }

    [Fact]
    public async Task RunUsesOutLangPromptNextToBspWhenPromptFileIsMissing()
    {
        using var temp = new TempDirectory();
        var bspPath = Path.Combine(temp.Path, "fake_map.bsp");
        File.WriteAllText(bspPath, "placeholder");
        File.WriteAllText(Path.Combine(temp.Path, "fake_map_prompt_schinese.md"), "Use schinese map prompt.");

        var llmClient = new FakeLLMClient();
        var runner = CreateRunner(llmClient);

        await runner.RunAsync(
            new LocalizationRequest(
                bspPath,
                "schinese",
                null,
                new LLMOptions("gpt-5.4", "sk-", null, null, "high", null)),
            null,
            CancellationToken.None);

        Assert.NotNull(llmClient.LastMessages);
        Assert.Equal("Use schinese map prompt.", llmClient.LastMessages[0].Content);
    }

    [Fact]
    public async Task RunUsesGenericPromptNextToBspWhenOutLangPromptIsMissing()
    {
        using var temp = new TempDirectory();
        var bspPath = Path.Combine(temp.Path, "fake_map.bsp");
        File.WriteAllText(bspPath, "placeholder");
        File.WriteAllText(Path.Combine(temp.Path, "fake_map_prompt.md"), "Use generic map prompt.");

        var llmClient = new FakeLLMClient();
        var runner = CreateRunner(llmClient);

        await runner.RunAsync(
            new LocalizationRequest(
                bspPath,
                "schinese",
                null,
                new LLMOptions("gpt-5.4", "sk-", null, null, "high", null)),
            null,
            CancellationToken.None);

        Assert.NotNull(llmClient.LastMessages);
        Assert.Equal("Use generic map prompt.", llmClient.LastMessages[0].Content);
    }

    [Fact]
    public async Task RunFallsBackToBuiltInPromptWhenDefaultPromptFilesAreMissing()
    {
        using var temp = new TempDirectory();
        var bspPath = Path.Combine(temp.Path, "fake_map.bsp");
        File.WriteAllText(bspPath, "placeholder");

        var llmClient = new FakeLLMClient();
        var runner = CreateRunner(llmClient);

        await runner.RunAsync(
            new LocalizationRequest(
                bspPath,
                "schinese",
                null,
                new LLMOptions("gpt-5.4", "sk-", null, null, "high", null)),
            null,
            CancellationToken.None);

        Assert.NotNull(llmClient.LastMessages);
        Assert.Equal(TranslationPromptBuilder.BuiltInPrompt, llmClient.LastMessages[0].Content);
    }

    [Fact]
    public async Task RunPrefersOutLangPromptOverGenericPromptNextToBsp()
    {
        using var temp = new TempDirectory();
        var bspPath = Path.Combine(temp.Path, "fake_map.bsp");
        File.WriteAllText(bspPath, "placeholder");
        File.WriteAllText(Path.Combine(temp.Path, "fake_map_prompt_schinese.md"), "Use schinese map prompt.");
        File.WriteAllText(Path.Combine(temp.Path, "fake_map_prompt.md"), "Use generic map prompt.");

        var llmClient = new FakeLLMClient();
        var runner = CreateRunner(llmClient);

        await runner.RunAsync(
            new LocalizationRequest(
                bspPath,
                "schinese",
                null,
                new LLMOptions("gpt-5.4", "sk-", null, null, "high", null)),
            null,
            CancellationToken.None);

        Assert.NotNull(llmClient.LastMessages);
        Assert.Equal("Use schinese map prompt.", llmClient.LastMessages[0].Content);
    }

    [Fact]
    public async Task RunPrefersExplicitPromptFileOverDefaultPromptNextToBsp()
    {
        using var temp = new TempDirectory();
        var bspPath = Path.Combine(temp.Path, "fake_map.bsp");
        var explicitPromptPath = Path.Combine(temp.Path, "explicit_prompt.md");
        File.WriteAllText(bspPath, "placeholder");
        File.WriteAllText(explicitPromptPath, "Use explicit prompt.");
        File.WriteAllText(Path.Combine(temp.Path, "fake_map_prompt_schinese.md"), "Use schinese map prompt.");

        var llmClient = new FakeLLMClient();
        var runner = CreateRunner(llmClient);

        await runner.RunAsync(
            new LocalizationRequest(
                bspPath,
                "schinese",
                explicitPromptPath,
                new LLMOptions("gpt-5.4", "sk-", null, null, "high", null)),
            null,
            CancellationToken.None);

        Assert.NotNull(llmClient.LastMessages);
        Assert.Equal("Use explicit prompt.", llmClient.LastMessages[0].Content);
    }

    private static LocalizationRunner CreateRunner(FakeLLMClient llmClient)
    {
        return new LocalizationRunner(
            new FakeExtractor([new GameTextEntry(0, "hello")]),
            llmClient,
            new DictionaryCsvWriter());
    }

    private sealed class FakeExtractor(IReadOnlyList<GameTextEntry> entries) : IGameTextExtractor
    {
        public IReadOnlyList<GameTextEntry> Extract(string bspPath) => entries;
    }

    private sealed class FakeLLMClient(string? response = null) : ILLMClient
    {
        public IReadOnlyList<LLMMessage>? LastMessages { get; private set; }

        public Task<string> CompleteTextAsync(
            IReadOnlyList<LLMMessage> messages,
            LLMOptions options,
            CancellationToken cancellationToken)
        {
            LastMessages = messages;

            return Task.FromResult(
                response ?? """{"translations":[{"id":0,"translation":"緊急事態だ!!\\n（出大事儿了！！）"}]}""");
        }
    }
}
