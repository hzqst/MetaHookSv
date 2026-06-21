using System.Globalization;

namespace BSPLocalizationTools.GUI.Lang;

public sealed class GuiLocalizer(Func<CultureInfo>? systemCultureProvider = null)
{
    public const string AutoLanguageCode = "auto";
    public const string EnglishLanguageCode = "en-US";
    public const string SimplifiedChineseLanguageCode = "zh-CN";
    public const string TraditionalChineseLanguageCode = "zh-TW";

    private readonly Func<CultureInfo> _systemCultureProvider = systemCultureProvider ?? (() => CultureInfo.CurrentUICulture);

    public string CurrentLanguageCode { get; private set; } = EnglishLanguageCode;

    public void SetLanguage(string? configuredLanguage)
    {
        CurrentLanguageCode = ResolveEffectiveLanguageCode(configuredLanguage, _systemCultureProvider());
        Resources.Culture = CultureInfo.GetCultureInfo(CurrentLanguageCode);
    }

    public static string NormalizeConfiguredLanguage(string? configuredLanguage)
    {
        if (string.IsNullOrWhiteSpace(configuredLanguage))
        {
            return AutoLanguageCode;
        }

        var language = configuredLanguage.Trim();
        return language switch
        {
            AutoLanguageCode => AutoLanguageCode,
            EnglishLanguageCode => EnglishLanguageCode,
            SimplifiedChineseLanguageCode => SimplifiedChineseLanguageCode,
            TraditionalChineseLanguageCode => TraditionalChineseLanguageCode,
            _ => AutoLanguageCode,
        };
    }

    public static string ResolveEffectiveLanguageCode(string? configuredLanguage, CultureInfo systemCulture)
    {
        var normalized = NormalizeConfiguredLanguage(configuredLanguage);
        if (normalized != AutoLanguageCode)
        {
            return normalized;
        }

        var cultureName = systemCulture.Name;
        var languageName = string.IsNullOrWhiteSpace(cultureName)
            ? systemCulture.TwoLetterISOLanguageName
            : cultureName;
        if (IsSimplifiedChinese(languageName))
        {
            return SimplifiedChineseLanguageCode;
        }

        if (IsTraditionalChinese(languageName))
        {
            return TraditionalChineseLanguageCode;
        }

        return EnglishLanguageCode;
    }

    public string GetStageText(TranslationStage stage)
    {
        return stage switch
        {
            TranslationStage.Queued => Resources.StatusQueued,
            TranslationStage.ExtractingGameText => Resources.StatusExtractingGameText,
            TranslationStage.BuildingPrompt => Resources.StatusBuildingPrompt,
            TranslationStage.RequestingTranslation => Resources.StatusRequestingTranslation,
            TranslationStage.ParsingResponse => Resources.StatusParsingResponse,
            TranslationStage.WritingDictionary => Resources.StatusWritingDictionary,
            TranslationStage.Completed => Resources.StatusCompleted,
            TranslationStage.Failed => Resources.StatusFailed,
            TranslationStage.Canceled => Resources.StatusCanceled,
            TranslationStage.Skipped => Resources.StatusSkipped,
            _ => stage.ToString(),
        };
    }

    private static bool IsSimplifiedChinese(string cultureName)
    {
        return cultureName.Equals("zh-CN", StringComparison.OrdinalIgnoreCase)
            || cultureName.Equals("zh-SG", StringComparison.OrdinalIgnoreCase)
            || cultureName.Equals("zh-Hans", StringComparison.OrdinalIgnoreCase)
            || cultureName.StartsWith("zh-Hans-", StringComparison.OrdinalIgnoreCase);
    }

    private static bool IsTraditionalChinese(string cultureName)
    {
        return cultureName.Equals("zh-TW", StringComparison.OrdinalIgnoreCase)
            || cultureName.Equals("zh-HK", StringComparison.OrdinalIgnoreCase)
            || cultureName.Equals("zh-MO", StringComparison.OrdinalIgnoreCase)
            || cultureName.Equals("zh-Hant", StringComparison.OrdinalIgnoreCase)
            || cultureName.StartsWith("zh-Hant-", StringComparison.OrdinalIgnoreCase);
    }
}
