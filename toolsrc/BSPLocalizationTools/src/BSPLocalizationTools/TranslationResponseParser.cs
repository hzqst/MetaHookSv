using System.Text.Json;

namespace BSPLocalizationTools;

public static class TranslationResponseParser
{
    public static IReadOnlyDictionary<int, string> Parse(string responseText, int expectedCount)
    {
        using var document = ParseDocument(StripCodeFence(responseText));
        if (!document.RootElement.TryGetProperty("translations", out var translations) ||
            translations.ValueKind != JsonValueKind.Array)
        {
            throw new InvalidOperationException("LLM response must contain a translations array.");
        }

        var result = new Dictionary<int, string>();
        foreach (var item in translations.EnumerateArray())
        {
            var id = item.GetProperty("id").GetInt32();
            var translation = item.GetProperty("translation").GetString();
            if (string.IsNullOrWhiteSpace(translation))
            {
                throw new InvalidOperationException($"LLM response translation for id {id} is empty.");
            }

            result[id] = translation;
        }

        for (var i = 0; i < expectedCount; i++)
        {
            if (!result.ContainsKey(i))
            {
                throw new InvalidOperationException($"LLM response missing id {i}.");
            }
        }

        return result;
    }

    private static JsonDocument ParseDocument(string json)
    {
        try
        {
            return JsonDocument.Parse(json);
        }
        catch (JsonException ex)
        {
            throw new InvalidOperationException("LLM response must be valid JSON.", ex);
        }
    }

    private static string StripCodeFence(string text)
    {
        var trimmed = text.Trim();
        if (!trimmed.StartsWith("```", StringComparison.Ordinal))
        {
            return trimmed;
        }

        var firstLineEnd = trimmed.IndexOf('\n');
        var lastFence = trimmed.LastIndexOf("```", StringComparison.Ordinal);
        if (firstLineEnd < 0 || lastFence <= firstLineEnd)
        {
            return trimmed;
        }

        return trimmed[(firstLineEnd + 1)..lastFence].Trim();
    }
}
