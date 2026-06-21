namespace BSPLocalizationTools.Tests;

public sealed class ToolConfigurationFileTests
{
    [Fact]
    public void SaveThenLoadRoundTripsKnownSettings()
    {
        using var temp = new TempDirectory();
        var envPath = Path.Combine(temp.Path, ".env");
        var configuration = new ToolConfiguration(
            new LLMOptions("gpt-5.5", "sk-test", "https://example.test/v1", 0.2, "high", "codex"),
            "tchinese",
            "prompts\\default.md");

        ToolConfigurationFile.Save(envPath, configuration);
        var loaded = ToolConfigurationFile.Load(envPath);

        Assert.Equal("gpt-5.5", loaded.LLM.Model);
        Assert.Equal("sk-test", loaded.LLM.ApiKey);
        Assert.Equal("https://example.test/v1", loaded.LLM.BaseUrl);
        Assert.Equal(0.2, loaded.LLM.Temperature);
        Assert.Equal("high", loaded.LLM.Effort);
        Assert.Equal("codex", loaded.LLM.FakeAs);
        Assert.Equal("tchinese", loaded.DefaultOutLang);
        Assert.Equal("prompts\\default.md", loaded.DefaultPromptFilePath);
    }

    [Fact]
    public void LoadMissingFileReturnsDefaults()
    {
        using var temp = new TempDirectory();

        var loaded = ToolConfigurationFile.Load(Path.Combine(temp.Path, ".env"));

        Assert.Equal("schinese", loaded.DefaultOutLang);
        Assert.Equal("medium", loaded.LLM.Effort);
        Assert.Null(loaded.DefaultPromptFilePath);
    }
}
