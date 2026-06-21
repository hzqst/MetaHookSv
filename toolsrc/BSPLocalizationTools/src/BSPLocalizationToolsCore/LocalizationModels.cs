namespace BSPLocalizationTools;

public sealed record LocalizationRequest(
    string BspPath,
    string OutLang,
    string? PromptFilePath,
    LLMOptions LLM,
    bool AppendLanguageToCsvFileName = true);

public sealed record LocalizationBatchRequest(IReadOnlyList<LocalizationRequest> Items);

public sealed record LocalizationResult(string BspPath, string OutputPath);

public sealed record LocalizationBatchItemResult(
    string BspPath,
    string? OutputPath,
    bool Succeeded,
    string? ErrorMessage);

public enum TranslationStage
{
    Queued,
    ExtractingGameText,
    BuildingPrompt,
    RequestingTranslation,
    ParsingResponse,
    WritingDictionary,
    Completed,
    Failed,
    Canceled,
}

public sealed record TranslationProgress(
    TranslationStage Stage,
    string BspPath,
    int ItemIndex,
    int ItemCount,
    string Message);

public sealed record ToolConfiguration(
    LLMOptions LLM,
    string DefaultOutLang,
    string? DefaultPromptFilePath,
    string GuiLanguage = "auto",
    bool AppendLanguageToCsvFileName = true)
{
    public static ToolConfiguration Default { get; } = new(
        new LLMOptions(null, null, null, null, "medium", null),
        "schinese",
        null,
        "auto",
        true);
}
