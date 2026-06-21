using System.Globalization;
using BSPLocalizationTools;

namespace BSPLocalizationTools.CLI;

public sealed record CommandLineOptions(
    string BspPath,
    string OutLang,
    string? PromptFilePath,
    LLMOptions LLM)
{
    private static readonly HashSet<string> KnownKeys = new(StringComparer.OrdinalIgnoreCase)
    {
        "bsp", "outlang", "promptfile", "llm_model", "llm_apikey",
        "llm_baseurl", "llm_temperature", "llm_effort", "llm_fake_as",
    };

    public static CommandLineOptions Parse(string[] args, Func<string, string?> getEnv)
    {
        var values = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        foreach (var arg in args)
        {
            if (!arg.StartsWith("-", StringComparison.Ordinal) || !arg.Contains('='))
            {
                throw new ArgumentException($"Invalid argument '{arg}'. Expected -key=value.");
            }

            var trimmed = arg.TrimStart('-');
            var separator = trimmed.IndexOf('=');
            var key = trimmed[..separator];
            var value = trimmed[(separator + 1)..];
            if (!KnownKeys.Contains(key))
            {
                throw new ArgumentException($"Unknown argument '-{key}'.");
            }

            values[key] = value;
        }

        var bspPath = GetRequired(values, "bsp");
        var outLang = GetOptional(values, "outlang") ?? "schinese";
        var promptFile = GetOptional(values, "promptfile");

        return new CommandLineOptions(
            bspPath,
            outLang,
            promptFile,
            new LLMOptions(
                GetOptional(values, "llm_model") ?? getEnv("BSPL10N_LLM_MODEL"),
                GetOptional(values, "llm_apikey") ?? getEnv("BSPL10N_LLM_APIKEY"),
                GetOptional(values, "llm_baseurl") ?? getEnv("BSPL10N_LLM_BASEURL"),
                ParseNullableDouble(GetOptional(values, "llm_temperature") ?? getEnv("BSPL10N_LLM_TEMPERATURE")),
                GetOptional(values, "llm_effort") ?? getEnv("BSPL10N_LLM_EFFORT") ?? "medium",
                NormalizeFakeAs(GetOptional(values, "llm_fake_as") ?? getEnv("BSPL10N_LLM_FAKE_AS"))));
    }

    public LocalizationRequest ToRequest()
    {
        return new LocalizationRequest(BspPath, OutLang, PromptFilePath, LLM);
    }

    private static string GetRequired(Dictionary<string, string> values, string key)
    {
        var value = GetOptional(values, key);
        if (string.IsNullOrWhiteSpace(value))
        {
            throw new ArgumentException($"Required argument '-{key}=...' is missing.");
        }

        return value;
    }

    private static string? GetOptional(Dictionary<string, string> values, string key)
    {
        return values.TryGetValue(key, out var value) && !string.IsNullOrWhiteSpace(value)
            ? value.Trim()
            : null;
    }

    private static double? ParseNullableDouble(string? value)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            return null;
        }

        if (!double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out var parsed))
        {
            throw new ArgumentException("LLM temperature must be a number.");
        }

        return parsed;
    }

    private static string? NormalizeFakeAs(string? value)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            return null;
        }

        var normalized = value.Trim().ToLowerInvariant();
        if (normalized != "codex")
        {
            throw new ArgumentException("Only '-llm_fake_as=codex' is supported.");
        }

        return normalized;
    }
}
