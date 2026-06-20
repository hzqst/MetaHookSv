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
