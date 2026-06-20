namespace BSPLocalizationTools.Tests;

public sealed class TranslationResponseParserTests
{
    [Fact]
    public void ParseReadsTranslationsById()
    {
        var parsed = TranslationResponseParser.Parse(
            """
            {"translations":[{"id":0,"translation":"你好"},{"id":1,"translation":"再见"}]}
            """,
            expectedCount: 2);

        Assert.Equal("你好", parsed[0]);
        Assert.Equal("再见", parsed[1]);
    }

    [Fact]
    public void ParseRejectsMissingTranslation()
    {
        var ex = Assert.Throws<InvalidOperationException>(() =>
            TranslationResponseParser.Parse("""{"translations":[{"id":0,"translation":"你好"}]}""", 2));

        Assert.Contains("missing id 1", ex.Message);
    }

    [Fact]
    public void ParseRejectsInvalidJsonWithClearError()
    {
        var ex = Assert.Throws<InvalidOperationException>(() =>
            TranslationResponseParser.Parse("not json", expectedCount: 1));

        Assert.Contains("valid JSON", ex.Message);
    }
}
