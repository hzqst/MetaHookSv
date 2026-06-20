using System.Text.Json;

namespace BSPLocalizationTools;

public static class TranslationPromptBuilder
{
    public const string BuiltInPrompt = """
        Translate GoldSrc/Sven Co-op game_text messages into the requested target language.
        Preserve literal \n sequences, punctuation intensity, urgency, and proper nouns when appropriate.
        Return natural in-game text, not explanatory notes.
        """;

    public static IReadOnlyList<LLMMessage> Build(
        string targetLanguage,
        string? customPrompt,
        IReadOnlyList<string> sourceMessages)
    {
        var instructions = string.IsNullOrWhiteSpace(customPrompt)
            ? BuiltInPrompt
            : customPrompt.Trim();
        var payload = new
        {
            target_language = targetLanguage,
            output_contract = new
            {
                format = "json",
                schema = """{"translations":[{"id":0,"translation":"..."}]}""",
                rules = new[]
                {
                    "Return only valid JSON.",
                    "Include every input id exactly once.",
                    "Keep literal \\n as backslash-n text.",
                },
            },
            inputs = sourceMessages.Select((text, id) => new { id, text }).ToArray(),
        };

        return
        [
            new LLMMessage("system", instructions),
            new LLMMessage("user", JsonSerializer.Serialize(payload)),
        ];
    }
}
