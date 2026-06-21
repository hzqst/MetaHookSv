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
        var promptPath = Path.Combine(temp.Path, "prompt.md");
        File.WriteAllText(promptPath, "Use saved prompt.");
        ToolConfigurationFile.Save(
            envPath,
            new ToolConfiguration(
                new LLMOptions("gpt-test", "sk-test", "https://example.test/v1", 0.1, "high", "codex"),
                "tchinese",
                promptPath,
                "zh-TW",
                AppendLanguageToCsvFileName: false));
        var vm = CreateViewModel(envPath, []);

        vm.LoadConfiguration();

        Assert.Equal("gpt-test", vm.LlmModel);
        Assert.Equal("sk-test", vm.LlmApiKey);
        Assert.Equal("https://example.test/v1", vm.LlmBaseUrl);
        Assert.Equal("0.1", vm.LlmTemperature);
        Assert.Equal("high", vm.LlmEffort);
        Assert.Equal("codex", vm.LlmFakeAs);
        Assert.Equal("tchinese", vm.OutLang);
        Assert.Equal(promptPath, vm.PromptFilePath);
        Assert.Equal("Use saved prompt.", vm.PromptText);
        Assert.Equal("zh-TW", vm.SelectedGuiLanguage.Code);
        Assert.Equal("翻譯", vm.Strings.TranslateTab);
        Assert.False(vm.AppendLanguageToCsvFileName);
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
        vm.AppendLanguageToCsvFileName = false;

        vm.SaveConfiguration();
        var loaded = ToolConfigurationFile.Load(envPath);

        Assert.Equal("gpt-save", loaded.LLM.Model);
        Assert.Equal("sk-save", loaded.LLM.ApiKey);
        Assert.Equal("schinese", loaded.DefaultOutLang);
        Assert.Equal("prompt.md", loaded.DefaultPromptFilePath);
        Assert.Equal("zh-CN", loaded.GuiLanguage);
        Assert.False(loaded.AppendLanguageToCsvFileName);
    }

    [Fact]
    public void LoadConfigurationUsesBuiltInPromptWhenPromptFileIsMissing()
    {
        using var temp = new TempDirectory();
        var envPath = Path.Combine(temp.Path, ".env");
        ToolConfigurationFile.Save(
            envPath,
            new ToolConfiguration(
                ToolConfiguration.Default.LLM,
                "schinese",
                Path.Combine(temp.Path, "missing.md")));
        var vm = CreateViewModel(envPath, []);

        vm.LoadConfiguration();

        Assert.Equal(TranslationPromptBuilder.BuiltInPrompt, vm.PromptText);
    }

    [Fact]
    public void ReloadPromptFromFileFallsBackToBuiltInPromptWhenPathIsBlank()
    {
        var vm = CreateViewModel("unused.env", []);
        vm.PromptText = "Custom prompt";
        vm.PromptFilePath = "";

        vm.ReloadPromptFromFile();

        Assert.Equal(TranslationPromptBuilder.BuiltInPrompt, vm.PromptText);
    }

    [Fact]
    public void LoadBuiltInPromptReplacesEditedPrompt()
    {
        var vm = CreateViewModel("unused.env", []);
        vm.PromptText = "Custom prompt";

        vm.LoadBuiltInPrompt();

        Assert.Equal(TranslationPromptBuilder.BuiltInPrompt, vm.PromptText);
    }

    [Fact]
    public void SavePromptToFileWritesUtf8PromptAndUpdatesConfigurationPath()
    {
        using var temp = new TempDirectory();
        var envPath = Path.Combine(temp.Path, ".env");
        var promptPath = Path.Combine(temp.Path, "prompts", "custom.md");
        var vm = CreateViewModel(envPath, []);
        vm.PromptText = "Line 1\r\nLine 2";

        vm.SavePromptToFile(promptPath);

        Assert.Equal(promptPath, vm.PromptFilePath);
        Assert.Equal("Line 1\r\nLine 2", File.ReadAllText(promptPath));

        var bytes = File.ReadAllBytes(promptPath);
        Assert.False(bytes.Length >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF);

        var loaded = ToolConfigurationFile.Load(envPath);
        Assert.Equal(promptPath, loaded.DefaultPromptFilePath);
    }

    [Fact]
    public void SavePromptUsesExistingPromptFilePath()
    {
        using var temp = new TempDirectory();
        var promptPath = Path.Combine(temp.Path, "prompt.md");
        var vm = CreateViewModel(Path.Combine(temp.Path, ".env"), []);
        vm.PromptFilePath = promptPath;
        vm.PromptText = "Existing path prompt.";

        Assert.True(vm.SavePrompt());

        Assert.Equal("Existing path prompt.", File.ReadAllText(promptPath));
    }

    [Fact]
    public void SavePromptReturnsFalseWhenPromptFilePathIsBlank()
    {
        var vm = CreateViewModel("unused.env", []);
        vm.PromptFilePath = "";
        vm.PromptText = "Unsaved prompt.";

        Assert.False(vm.SavePrompt());
    }

    [Fact]
    public void ChangingGuiLanguageUpdatesLocalizedStringsImmediately()
    {
        var vm = CreateViewModel("unused.env", []);

        Assert.Equal("Translate", vm.Strings.TranslateTab);
        Assert.Equal("Prompt editor", vm.Strings.PromptEditor);

        vm.SelectedGuiLanguage = vm.GuiLanguageOptions.Single(o => o.Code == "zh-CN");

        Assert.Equal("翻译", vm.Strings.TranslateTab);
        Assert.Equal("设置", vm.Strings.SettingsTab);
        Assert.Equal("提示词编辑器", vm.Strings.PromptEditor);
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
    public void RemoveItemCommandRemovesSpecifiedTask()
    {
        var vm = CreateViewModel("unused.env", []);
        vm.AddBspFiles(["first.bsp", "second.bsp"]);

        vm.RemoveItemCommand.Execute(vm.Items[0]);

        Assert.Equal(["second.bsp"], vm.Items.Select(i => i.BspPath).ToArray());
    }

    [Fact]
    public void ClearCompletedItemsCommandRemovesOnlyCompletedTasks()
    {
        var vm = CreateViewModel("unused.env", []);
        vm.AddBspFiles(["completed.bsp", "queued.bsp", "failed.bsp", "canceled.bsp", "running.bsp"]);
        vm.Items[0].Stage = TranslationStage.Completed;
        vm.Items[2].Stage = TranslationStage.Failed;
        vm.Items[3].Stage = TranslationStage.Canceled;
        vm.Items[4].Stage = TranslationStage.RequestingTranslation;

        vm.ClearCompletedItemsCommand.Execute(null);

        Assert.Equal(["queued.bsp", "failed.bsp", "canceled.bsp", "running.bsp"], vm.Items.Select(i => i.BspPath).ToArray());
    }

    [Fact]
    public void ClearAllItemsCommandRemovesEveryTask()
    {
        var vm = CreateViewModel("unused.env", []);
        vm.AddBspFiles(["first.bsp", "second.bsp"]);

        vm.ClearAllItemsCommand.Execute(null);

        Assert.Empty(vm.Items);
    }

    [Fact]
    public void GetRawGameTextFormatsExtractedMessages()
    {
        var vm = CreateViewModel(
            "unused.env",
            [],
            new ConfigurableExtractor([new GameTextEntry(0, "hello"), new GameTextEntry(1, "line 1\\nline 2")]));
        vm.AddBspFiles(["map.bsp"]);

        var text = vm.GetRawGameText(vm.Items[0]);

        Assert.Contains("[0] hello", text);
        Assert.Contains("[1] line 1\\nline 2", text);
    }

    [Fact]
    public void GetRawGameTextReturnsLocalizedEmptyMessageWhenNoEntriesWereFound()
    {
        var vm = CreateViewModel("unused.env", [], new ConfigurableExtractor([]));
        vm.SelectedGuiLanguage = vm.GuiLanguageOptions.Single(o => o.Code == "zh-CN");
        vm.AddBspFiles(["empty.bsp"]);

        var text = vm.GetRawGameText(vm.Items[0]);

        Assert.Equal(vm.Strings.NoGameTextFound, text);
    }

    [Fact]
    public void GetRawGameTextReturnsLocalizedFailureWhenExtractionFails()
    {
        var vm = CreateViewModel("unused.env", [], new ConfigurableExtractor(null, new InvalidOperationException("boom")));
        vm.AddBspFiles(["broken.bsp"]);

        var text = vm.GetRawGameText(vm.Items[0]);

        Assert.Contains("Failed to load game_text", text);
        Assert.Contains("boom", text);
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

    [Fact]
    public async Task StartTranslationKeepsSkippedItemsSkipped()
    {
        using var temp = new TempDirectory();
        var bspPath = Path.Combine(temp.Path, "empty.bsp");
        File.WriteAllText(bspPath, "placeholder");
        var vm = CreateViewModel(
            Path.Combine(temp.Path, ".env"),
            [new LocalizationBatchItemResult(bspPath, null, true, null, Skipped: true)]);
        vm.AddBspFiles([bspPath]);

        await vm.StartTranslationAsync();

        Assert.Equal(TranslationStage.Skipped, vm.Items[0].Stage);
        Assert.Equal("Skipped", vm.Items[0].Status);
        Assert.Null(vm.Items[0].OutputPath);
    }

    [Fact]
    public async Task StartTranslationPassesAppendLanguageToCsvFileNameToRequests()
    {
        using var temp = new TempDirectory();
        var bspPath = Path.Combine(temp.Path, "map.bsp");
        File.WriteAllText(bspPath, "placeholder");
        var requests = new List<LocalizationRequest>();
        var vm = CreateViewModel(
            Path.Combine(temp.Path, ".env"),
            [new LocalizationBatchItemResult(bspPath, "map_dictionary.csv", true, null)],
            requestObserver: requests.Add);
        vm.AppendLanguageToCsvFileName = false;
        vm.AddBspFiles([bspPath]);

        await vm.StartTranslationAsync();

        var request = Assert.Single(requests);
        Assert.False(request.AppendLanguageToCsvFileName);
    }

    private static MainWindowViewModel CreateViewModel(
        string envPath,
        IReadOnlyList<LocalizationBatchItemResult> results,
        IGameTextExtractor? extractor = null,
        Action<LocalizationRequest>? requestObserver = null)
    {
        return new MainWindowViewModel(
            new BatchLocalizationRunner(new FakeRunner(results, requestObserver)),
            envPath,
            () => CultureInfo.GetCultureInfo("en-US"),
            extractor);
    }

    private sealed class FakeRunner(
        IReadOnlyList<LocalizationBatchItemResult> results,
        Action<LocalizationRequest>? requestObserver) : LocalizationRunner(
        new FakeExtractor(),
        new FakeLLMClient(),
        new DictionaryCsvWriter())
    {
        public override Task<LocalizationResult> RunAsync(
            LocalizationRequest request,
            IProgress<TranslationProgress>? progress,
            CancellationToken cancellationToken)
        {
            requestObserver?.Invoke(request);
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

            if (result is { Succeeded: true, Skipped: true })
            {
                progress?.Report(new TranslationProgress(
                    TranslationStage.Skipped,
                    request.BspPath,
                    1,
                    1,
                    "Skipped empty game_text messages."));
                return Task.FromResult(new LocalizationResult(request.BspPath, null, Skipped: true));
            }

            throw new InvalidOperationException(result?.ErrorMessage ?? "failed");
        }
    }

    private sealed class FakeExtractor : IGameTextExtractor
    {
        public IReadOnlyList<GameTextEntry> Extract(string bspPath) => [new GameTextEntry(0, "hello")];
    }

    private sealed class ConfigurableExtractor(
        IReadOnlyList<GameTextEntry>? entries,
        Exception? exception = null) : IGameTextExtractor
    {
        public IReadOnlyList<GameTextEntry> Extract(string bspPath)
        {
            if (exception is not null)
            {
                throw exception;
            }

            return entries ?? [];
        }
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
