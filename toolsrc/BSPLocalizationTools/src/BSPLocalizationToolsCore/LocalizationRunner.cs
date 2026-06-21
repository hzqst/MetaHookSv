namespace BSPLocalizationTools;

public class LocalizationRunner(
    IGameTextExtractor extractor,
    ILLMClient llmClient,
    DictionaryCsvWriter csvWriter)
{
    public virtual async Task<LocalizationResult> RunAsync(
        LocalizationRequest request,
        IProgress<TranslationProgress>? progress,
        CancellationToken cancellationToken)
    {
        if (!File.Exists(request.BspPath))
        {
            throw new FileNotFoundException("BSP file was not found.", request.BspPath);
        }

        Report(progress, request.BspPath, TranslationStage.ExtractingGameText, "Extracting game_text messages.");
        cancellationToken.ThrowIfCancellationRequested();
        var prompt = ReadPrompt(request.BspPath, request.OutLang, request.PromptFilePath);
        var entries = extractor.Extract(request.BspPath);
        if (entries.Count == 0)
        {
            throw new InvalidOperationException("No game_text messages were found.");
        }

        Report(progress, request.BspPath, TranslationStage.BuildingPrompt, "Building translation prompt.");
        var uniqueMessages = entries.Select(e => e.Message).Distinct(StringComparer.Ordinal).ToArray();
        var llmMessages = TranslationPromptBuilder.Build(request.OutLang, prompt, uniqueMessages);

        Report(progress, request.BspPath, TranslationStage.RequestingTranslation, "Requesting LLM translation.");
        var response = await llmClient.CompleteTextAsync(llmMessages, request.LLM, cancellationToken);

        Report(progress, request.BspPath, TranslationStage.ParsingResponse, "Parsing LLM response.");
        var translations = TranslationResponseParser.Parse(response, uniqueMessages.Length);
        var translatedBySource = uniqueMessages
            .Select((source, id) => new { source, translation = translations[id] })
            .ToDictionary(x => x.source, x => x.translation, StringComparer.Ordinal);
        var rows = entries
            .Where(e => !string.Equals(e.Message, translatedBySource[e.Message], StringComparison.Ordinal))
            .Select(e => new DictionaryRow("NETMESSAGE:" + e.Message, translatedBySource[e.Message]))
            .ToArray();

        Report(progress, request.BspPath, TranslationStage.WritingDictionary, "Writing dictionary CSV.");
        var outputPath = GetOutputPath(request.BspPath, request.OutLang);
        csvWriter.Write(outputPath, request.OutLang, rows);
        Report(progress, request.BspPath, TranslationStage.Completed, $"Wrote dictionary: {outputPath}");
        return new LocalizationResult(request.BspPath, outputPath);
    }

    public static string GetOutputPath(string bspPath, string outLang)
    {
        var directory = Path.GetDirectoryName(Path.GetFullPath(bspPath)) ?? Environment.CurrentDirectory;
        var mapName = Path.GetFileNameWithoutExtension(bspPath);
        return Path.Combine(directory, $"{mapName}_dictionary_{outLang}.csv");
    }

    private static void Report(
        IProgress<TranslationProgress>? progress,
        string bspPath,
        TranslationStage stage,
        string message)
    {
        progress?.Report(new TranslationProgress(stage, bspPath, 1, 1, message));
    }

    private static string? ReadPrompt(string bspPath, string outLang, string? promptFilePath)
    {
        if (!string.IsNullOrWhiteSpace(promptFilePath))
        {
            if (!File.Exists(promptFilePath))
            {
                throw new FileNotFoundException("Prompt file was not found.", promptFilePath);
            }

            return File.ReadAllText(promptFilePath);
        }

        foreach (var defaultPromptPath in GetDefaultPromptPaths(bspPath, outLang))
        {
            if (File.Exists(defaultPromptPath))
            {
                return File.ReadAllText(defaultPromptPath);
            }
        }

        return null;
    }

    private static IEnumerable<string> GetDefaultPromptPaths(string bspPath, string outLang)
    {
        var fullBspPath = Path.GetFullPath(bspPath);
        var directory = Path.GetDirectoryName(fullBspPath) ?? Environment.CurrentDirectory;
        var mapName = Path.GetFileNameWithoutExtension(fullBspPath);

        yield return Path.Combine(directory, $"{mapName}_prompt_{outLang}.md");
        yield return Path.Combine(directory, $"{mapName}_prompt.md");
    }
}
