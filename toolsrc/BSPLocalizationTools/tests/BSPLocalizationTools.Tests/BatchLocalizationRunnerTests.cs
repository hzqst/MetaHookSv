namespace BSPLocalizationTools.Tests;

public sealed class BatchLocalizationRunnerTests
{
    [Fact]
    public async Task RunAsyncContinuesAfterItemFailure()
    {
        using var temp = new TempDirectory();
        var first = Path.Combine(temp.Path, "first.bsp");
        var second = Path.Combine(temp.Path, "second.bsp");
        File.WriteAllText(first, "placeholder");
        File.WriteAllText(second, "placeholder");
        var progressEvents = new List<TranslationProgress>();
        var runner = new BatchLocalizationRunner(CreateRunner());

        var results = await runner.RunAsync(
            new LocalizationBatchRequest(
                [
                    new LocalizationRequest(first, "schinese", null, CreateOptions()),
                    new LocalizationRequest(second, "schinese", null, CreateOptions()),
                ]),
            new Progress<TranslationProgress>(progressEvents.Add),
            CancellationToken.None);

        Assert.False(results[0].Succeeded);
        Assert.True(results[1].Succeeded);
        Assert.Equal(2, results.Count);
        Assert.Contains(progressEvents, e => e.BspPath == second && e.Stage == TranslationStage.Completed);
    }

    [Fact]
    public async Task RunAsyncStopsStartingNewItemsAfterCancellation()
    {
        using var temp = new TempDirectory();
        var first = Path.Combine(temp.Path, "first.bsp");
        var second = Path.Combine(temp.Path, "second.bsp");
        File.WriteAllText(first, "placeholder");
        File.WriteAllText(second, "placeholder");
        using var cts = new CancellationTokenSource();
        var runner = new BatchLocalizationRunner(new CancellingRunner(cts));

        var results = await runner.RunAsync(
            new LocalizationBatchRequest(
                [
                    new LocalizationRequest(first, "schinese", null, CreateOptions()),
                    new LocalizationRequest(second, "schinese", null, CreateOptions()),
                ]),
            null,
            cts.Token);

        Assert.Single(results);
        Assert.Equal(first, results[0].BspPath);
        Assert.False(results[0].Succeeded);
    }

    private static LLMOptions CreateOptions()
    {
        return new LLMOptions("gpt-test", "sk-", null, null, "medium", null);
    }

    private static LocalizationRunner CreateRunner()
    {
        return new LocalizationRunner(
            new PathBasedExtractor(),
            new FakeLLMClient(),
            new DictionaryCsvWriter());
    }

    private sealed class PathBasedExtractor : IGameTextExtractor
    {
        public IReadOnlyList<GameTextEntry> Extract(string bspPath)
        {
            if (Path.GetFileNameWithoutExtension(bspPath) == "first")
            {
                return [];
            }

            return [new GameTextEntry(0, "hello")];
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

    private sealed class CancellingRunner(CancellationTokenSource source) : LocalizationRunner(
        new PathBasedExtractor(),
        new FakeLLMClient(),
        new DictionaryCsvWriter())
    {
        public override Task<LocalizationResult> RunAsync(
            LocalizationRequest request,
            IProgress<TranslationProgress>? progress,
            CancellationToken cancellationToken)
        {
            source.Cancel();
            cancellationToken.ThrowIfCancellationRequested();
            return Task.FromResult(new LocalizationResult(request.BspPath, ""));
        }
    }
}
