namespace BSPLocalizationTools.Tests;

public sealed class TranslationPromptBuilderTests
{
    [Fact]
    public void BuildIncludesCustomPromptLanguageAndInputs()
    {
        var messages = TranslationPromptBuilder.Build(
            "schinese",
            "Translate with Sven Co-op tone.",
            ["hello\\nworld"]);

        Assert.Equal("system", messages[0].Role);
        Assert.Equal("user", messages[1].Role);
        Assert.Contains("Translate with Sven Co-op tone.", messages[0].Content);
        Assert.Contains("\"target_language\":\"schinese\"", messages[1].Content);
        Assert.Contains("\"id\":0", messages[1].Content);
        Assert.Contains("hello\\\\nworld", messages[1].Content);
    }
}
