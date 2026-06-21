using BSPLocalizationTools.GUI.ViewModels;
using System.Globalization;

namespace BSPLocalizationTools.Tests;

public sealed class MainWindowViewModelTests
{
    [Fact]
    public void LoadConfigurationFillsSettingsFields()
    {
        using var temp = new TempDirectory();
        var envPath = Path.Combine(temp.Path, ".env");
        ToolConfigurationFile.Save(
            envPath,
            new ToolConfiguration(
                new LLMOptions("gpt-test", "sk-test", "https://example.test/v1", 0.1, "high", "codex"),
                "tchinese",
                "prompt.md",
                "zh-TW"));
        var vm = CreateViewModel(envPath, []);

        vm.LoadConfiguration();

        Assert.Equal("gpt-test", vm.LlmModel);
        Assert.Equal("sk-test", vm.LlmApiKey);
        Assert.Equal("https://example.test/v1", vm.LlmBaseUrl);
        Assert.Equal("0.1", vm.LlmTemperature);
        Assert.Equal("high", vm.LlmEffort);
        Assert.Equal("codex", vm.LlmFakeAs);
        Assert.Equal("tchinese", vm.OutLang);
        Assert.Equal("prompt.md", vm.PromptFilePath);
        Assert.Equal("zh-TW", vm.SelectedGuiLanguage.Code);
        Assert.Equal("翻譯", vm.Strings.TranslateTab);
    }

    [Fact]
    public void SaveConfigurationWritesEnvFile()
    {
        using var temp = new TempDirectory();
        var envPath = Path.Combine(temp.Path, ".env");
        var vm = CreateViewModel(envPath, []);
        vm.LlmModel = "gpt-save";
        vm.LlmApiKey = "sk-save";
        vm.OutLang = "schinese";
        vm.PromptFilePath = "prompt.md";
        vm.SelectedGuiLanguage = vm.GuiLanguageOptions.Single(o => o.Code == "zh-CN");

        vm.SaveConfiguration();
        var loaded = ToolConfigurationFile.Load(envPath);

        Assert.Equal("gpt-save", loaded.LLM.Model);
        Assert.Equal("sk-save", loaded.LLM.ApiKey);
        Assert.Equal("schinese", loaded.DefaultOutLang);
        Assert.Equal("prompt.md", loaded.DefaultPromptFilePath);
        Assert.Equal("zh-CN", loaded.GuiLanguage);
    }

    [Fact]
    public void ChangingGuiLanguageUpdatesLocalizedStringsImmediately()
    {
        var vm = CreateViewModel("unused.env", []);

        Assert.Equal("Translate", vm.Strings.TranslateTab);

        vm.SelectedGuiLanguage = vm.GuiLanguageOptions.Single(o => o.Code == "zh-CN");

        Assert.Equal("翻译", vm.Strings.TranslateTab);
        Assert.Equal("设置", vm.Strings.SettingsTab);
    }

    [Fact]
    public async Task StartTranslationUpdatesStatusUsingSelectedGuiLanguage()
    {
        using var temp = new TempDirectory();
        var bspPath = Path.Combine(temp.Path, "map.bsp");
        File.WriteAllText(bspPath, "placeholder");
        var vm = CreateViewModel(
            Path.Combine(temp.Path, ".env"),
            [new LocalizationBatchItemResult(bspPath, "map_dictionary_schinese.csv", true, null)]);
        vm.SelectedGuiLanguage = vm.GuiLanguageOptions.Single(o => o.Code == "zh-TW");
        vm.AddBspFiles([bspPath]);

        await vm.StartTranslationAsync();

        Assert.Equal("已完成", vm.Items[0].Status);
    }

    [Fact]
    public void AddBspFilesKeepsOnlyUniqueBspPaths()
    {
        var vm = CreateViewModel("unused.env", []);

        vm.AddBspFiles(["a.bsp", "a.bsp", "readme.txt", "b.BSP"]);

        Assert.Equal(["a.bsp", "b.BSP"], vm.Items.Select(i => i.BspPath).ToArray());
    }

    [Fact]
    public async Task StartTranslationUpdatesItemStatusesAndOutputs()
    {
        using var temp = new TempDirectory();
        var bspPath = Path.Combine(temp.Path, "map.bsp");
        File.WriteAllText(bspPath, "placeholder");
        var vm = CreateViewModel(
            Path.Combine(temp.Path, ".env"),
            [new LocalizationBatchItemResult(bspPath, "map_dictionary_schinese.csv", true, null)]);
        vm.AddBspFiles([bspPath]);

        await vm.StartTranslationAsync();

        Assert.False(vm.IsTranslating);
        Assert.Equal("Completed", vm.Items[0].Status);
        Assert.Equal("map_dictionary_schinese.csv", vm.Items[0].OutputPath);
    }

    private static MainWindowViewModel CreateViewModel(
        string envPath,
        IReadOnlyList<LocalizationBatchItemResult> results)
    {
        return new MainWindowViewModel(
            new BatchLocalizationRunner(new FakeRunner(results)),
            envPath,
            () => CultureInfo.GetCultureInfo("en-US"));
    }

    private sealed class FakeRunner(IReadOnlyList<LocalizationBatchItemResult> results) : LocalizationRunner(
        new FakeExtractor(),
        new FakeLLMClient(),
        new DictionaryCsvWriter())
    {
        public override Task<LocalizationResult> RunAsync(
            LocalizationRequest request,
            IProgress<TranslationProgress>? progress,
            CancellationToken cancellationToken)
        {
            progress?.Report(new TranslationProgress(
                TranslationStage.Completed,
                request.BspPath,
                1,
                1,
                "done"));
            var result = results.FirstOrDefault(r => string.Equals(r.BspPath, request.BspPath, StringComparison.OrdinalIgnoreCase));
            if (result is { Succeeded: true, OutputPath: not null })
            {
                return Task.FromResult(new LocalizationResult(request.BspPath, result.OutputPath));
            }

            throw new InvalidOperationException(result?.ErrorMessage ?? "failed");
        }
    }

    private sealed class FakeExtractor : IGameTextExtractor
    {
        public IReadOnlyList<GameTextEntry> Extract(string bspPath) => [new GameTextEntry(0, "hello")];
    }

    private sealed class FakeLLMClient : ILLMClient
    {
        public Task<string> CompleteTextAsync(
            IReadOnlyList<LLMMessage> messages,
            LLMOptions options,
            CancellationToken cancellationToken)
        {
            return Task.FromResult("""{"translations":[{"id":0,"translation":"你好"}]}""");
        }
    }
}
