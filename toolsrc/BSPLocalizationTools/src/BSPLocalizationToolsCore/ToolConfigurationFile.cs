using System.Globalization;
using System.Text;

namespace BSPLocalizationTools;

public static class ToolConfigurationFile
{
    public const string LlmModelKey = "BSPL10N_LLM_MODEL";
    public const string LlmApiKeyKey = "BSPL10N_LLM_APIKEY";
    public const string LlmBaseUrlKey = "BSPL10N_LLM_BASEURL";
    public const string LlmTemperatureKey = "BSPL10N_LLM_TEMPERATURE";
    public const string LlmEffortKey = "BSPL10N_LLM_EFFORT";
    public const string LlmFakeAsKey = "BSPL10N_LLM_FAKE_AS";
    public const string DefaultOutLangKey = "BSPL10N_DEFAULT_OUTLANG";
    public const string DefaultPromptFileKey = "BSPL10N_DEFAULT_PROMPTFILE";
    public const string GuiLanguageKey = "BSPL10N_GUI_LANGUAGE";
    public const string AppendLanguageToCsvFileNameKey = "BSPL10N_APPEND_LANGUAGE_TO_CSV_FILENAME";

    public static string GetDefaultPath()
    {
        return Path.Combine(AppContext.BaseDirectory, ".env");
    }

    public static ToolConfiguration Load(string envPath)
    {
        if (!File.Exists(envPath))
        {
            return ToolConfiguration.Default;
        }

        var values = ReadValues(envPath);
        return new ToolConfiguration(
            new LLMOptions(
                Get(values, LlmModelKey),
                Get(values, LlmApiKeyKey),
                Get(values, LlmBaseUrlKey),
                ParseTemperature(Get(values, LlmTemperatureKey)),
            Get(values, LlmEffortKey) ?? ToolConfiguration.Default.LLM.Effort,
            Get(values, LlmFakeAsKey)),
            Get(values, DefaultOutLangKey) ?? ToolConfiguration.Default.DefaultOutLang,
            Get(values, DefaultPromptFileKey),
            Get(values, GuiLanguageKey) ?? ToolConfiguration.Default.GuiLanguage,
            ParseBoolean(Get(values, AppendLanguageToCsvFileNameKey)) ??
                ToolConfiguration.Default.AppendLanguageToCsvFileName);
    }

    public static void Save(string envPath, ToolConfiguration configuration)
    {
        var directory = Path.GetDirectoryName(Path.GetFullPath(envPath));
        if (!string.IsNullOrWhiteSpace(directory))
        {
            Directory.CreateDirectory(directory);
        }

        var lines = new List<string>
        {
            Format(LlmModelKey, configuration.LLM.Model),
            Format(LlmApiKeyKey, configuration.LLM.ApiKey),
            Format(LlmBaseUrlKey, configuration.LLM.BaseUrl),
            Format(
                LlmTemperatureKey,
                configuration.LLM.Temperature?.ToString(CultureInfo.InvariantCulture)),
            Format(LlmEffortKey, configuration.LLM.Effort),
            Format(LlmFakeAsKey, configuration.LLM.FakeAs),
            Format(DefaultOutLangKey, configuration.DefaultOutLang),
            Format(DefaultPromptFileKey, configuration.DefaultPromptFilePath),
            Format(GuiLanguageKey, configuration.GuiLanguage),
            Format(AppendLanguageToCsvFileNameKey, configuration.AppendLanguageToCsvFileName.ToString()),
        };
        File.WriteAllLines(envPath, lines, new UTF8Encoding(encoderShouldEmitUTF8Identifier: false));
    }

    private static Dictionary<string, string> ReadValues(string envPath)
    {
        var values = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        foreach (var rawLine in File.ReadAllLines(envPath))
        {
            var line = rawLine.Trim();
            if (line.Length == 0 || line.StartsWith('#'))
            {
                continue;
            }

            var separator = line.IndexOf('=');
            if (separator <= 0)
            {
                continue;
            }

            var key = line[..separator].Trim();
            var value = line[(separator + 1)..].Trim();
            values[key] = Unquote(value);
        }

        return values;
    }

    private static string? Get(Dictionary<string, string> values, string key)
    {
        return values.TryGetValue(key, out var value) && !string.IsNullOrWhiteSpace(value)
            ? value
            : null;
    }

    private static double? ParseTemperature(string? value)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            return null;
        }

        return double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out var parsed)
            ? parsed
            : throw new InvalidOperationException($"{LlmTemperatureKey} must be a number.");
    }

    private static bool? ParseBoolean(string? value)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            return null;
        }

        return bool.TryParse(value.Trim(), out var parsed)
            ? parsed
            : throw new InvalidOperationException($"{AppendLanguageToCsvFileNameKey} must be true or false.");
    }

    private static string Format(string key, string? value)
    {
        return $"{key}={Escape(value)}";
    }

    private static string Escape(string? value)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            return "";
        }

        if (!value.Contains('\r', StringComparison.Ordinal) && !value.Contains('\n', StringComparison.Ordinal))
        {
            return value;
        }

        var escaped = value
            .Replace("\r", "\\r", StringComparison.Ordinal)
            .Replace("\n", "\\n", StringComparison.Ordinal);
        return $"\"{escaped}\"";
    }

    private static string Unquote(string value)
    {
        if (value.Length >= 2 && value[0] == '"' && value[^1] == '"')
        {
            value = value[1..^1];
            return value
                .Replace("\\n", "\n", StringComparison.Ordinal)
                .Replace("\\r", "\r", StringComparison.Ordinal);
        }

        return value;
    }
}
