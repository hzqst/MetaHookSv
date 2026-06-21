namespace BSPLocalizationTools.Tests;

public sealed class LocalizationRunnerProgressTests
{
    [Fact]
    public async Task RunAsyncReportsStagesAndOutputPath()
    {
        using var temp = new TempDirectory();
        var bspPath = Path.Combine(temp.Path, "fake_map.bsp");
        File.WriteAllText(bspPath, "placeholder");
        var progressEvents = new List<TranslationProgress>();
        var runner = CreateRunner([
            new GameTextEntry(0, "hello"),
        ]);

        var result = await runner.RunAsync(
            new LocalizationRequest(
                bspPath,
                "schinese",
                null,
                new LLMOptions("gpt-test", "sk-", null, null, "medium", null)),
            new Progress<TranslationProgress>(progressEvents.Add),
            CancellationToken.None);

        Assert.Equal(Path.Combine(temp.Path, "fake_map_dictionary_schinese.csv"), result.OutputPath);
        Assert.Contains(progressEvents, e => e.Stage == TranslationStage.ExtractingGameText);
        Assert.Contains(progressEvents, e => e.Stage == TranslationStage.RequestingTranslation);
        Assert.Contains(progressEvents, e => e.Stage == TranslationStage.WritingDictionary);
        Assert.Equal(TranslationStage.Completed, progressEvents[^1].Stage);
    }

    private static LocalizationRunner CreateRunner(IReadOnlyList<GameTextEntry> entries)
    {
        return new LocalizationRunner(
            new FakeExtractor(entries),
            new FakeLLMClient(),
            new DictionaryCsvWriter());
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
            return Task.FromResult("""{"translations":[{"id":0,"translation":"你好"}]}""");
        }
    }
}
