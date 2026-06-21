namespace BSPLocalizationTools;

public sealed class BatchLocalizationRunner(LocalizationRunner runner)
{
    public async Task<IReadOnlyList<LocalizationBatchItemResult>> RunAsync(
        LocalizationBatchRequest request,
        IProgress<TranslationProgress>? progress,
        CancellationToken cancellationToken)
    {
        var results = new List<LocalizationBatchItemResult>();
        for (var i = 0; i < request.Items.Count; i++)
        {
            if (cancellationToken.IsCancellationRequested)
            {
                break;
            }

            var item = request.Items[i];
            Report(progress, TranslationStage.Queued, item.BspPath, i, request.Items.Count, "Queued translation.");
            try
            {
                var itemProgress = CreateItemProgress(progress, i, request.Items.Count);
                var result = await runner.RunAsync(item, itemProgress, cancellationToken);
                results.Add(new LocalizationBatchItemResult(item.BspPath, result.OutputPath, true, null));
            }
            catch (OperationCanceledException ex)
            {
                Report(progress, TranslationStage.Canceled, item.BspPath, i, request.Items.Count, "Translation canceled.");
                results.Add(new LocalizationBatchItemResult(item.BspPath, null, false, ex.Message));
                break;
            }
            catch (Exception ex)
            {
                Report(progress, TranslationStage.Failed, item.BspPath, i, request.Items.Count, ex.Message);
                results.Add(new LocalizationBatchItemResult(item.BspPath, null, false, ex.Message));
            }
        }

        return results;
    }

    private static IProgress<TranslationProgress>? CreateItemProgress(
        IProgress<TranslationProgress>? progress,
        int itemIndex,
        int itemCount)
    {
        return progress is null
            ? null
            : new ForwardingProgress(p =>
                Report(progress, p.Stage, p.BspPath, itemIndex, itemCount, p.Message));
    }

    private static void Report(
        IProgress<TranslationProgress>? progress,
        TranslationStage stage,
        string bspPath,
        int itemIndex,
        int itemCount,
        string message)
    {
        progress?.Report(new TranslationProgress(stage, bspPath, itemIndex + 1, itemCount, message));
    }

    private sealed class ForwardingProgress(Action<TranslationProgress> report) : IProgress<TranslationProgress>
    {
        public void Report(TranslationProgress value)
        {
            report(value);
        }
    }
}
