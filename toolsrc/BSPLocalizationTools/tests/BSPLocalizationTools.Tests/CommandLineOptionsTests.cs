namespace BSPLocalizationTools.Tests;

public sealed class CommandLineOptionsTests
{
    [Fact]
    public void ParseUsesDefaultsAndRequiredBsp()
    {
        var options = CommandLineOptions.Parse(
            ["-bsp=C:\\maps\\pizza.bsp"],
            _ => null);

        Assert.Equal("C:\\maps\\pizza.bsp", options.BspPath);
        Assert.Equal("schinese", options.OutLang);
        Assert.Null(options.PromptFilePath);
    }

    [Fact]
    public void ParseReadsLLMEnvironmentFallbacks()
    {
        var env = new Dictionary<string, string?>
        {
            ["BSPL10N_LLM_MODEL"] = "gpt-5.4",
            ["BSPL10N_LLM_APIKEY"] = "sk-",
            ["BSPL10N_LLM_BASEURL"] = "https://api.openai.com/v1",
            ["BSPL10N_LLM_TEMPERATURE"] = "0.2",
            ["BSPL10N_LLM_EFFORT"] = "high",
            ["BSPL10N_LLM_FAKE_AS"] = "codex",
        };

        var options = CommandLineOptions.Parse(
            ["-bsp=map.bsp", "-outlang=tchinese", "-promptfile=prompt.md"],
            key => env.TryGetValue(key, out var value) ? value : null);

        Assert.Equal("tchinese", options.OutLang);
        Assert.Equal("prompt.md", options.PromptFilePath);
        Assert.Equal("gpt-5.4", options.LLM.Model);
        Assert.Equal("sk-", options.LLM.ApiKey);
        Assert.Equal("https://api.openai.com/v1", options.LLM.BaseUrl);
        Assert.Equal(0.2, options.LLM.Temperature);
        Assert.Equal("high", options.LLM.Effort);
        Assert.Equal("codex", options.LLM.FakeAs);
    }

    [Fact]
    public void ParseRejectsUnknownArgument()
    {
        var ex = Assert.Throws<ArgumentException>(() =>
            CommandLineOptions.Parse(["-bsp=map.bsp", "-wat=no"], _ => null));

        Assert.Contains("Unknown argument", ex.Message);
    }

    [Fact]
    public void ToRequestMapsCliOptionsToCoreRequest()
    {
        var options = CommandLineOptions.Parse(
            ["-bsp=map.bsp", "-outlang=tchinese", "-promptfile=prompt.md", "-llm_model=gpt-test"],
            _ => null);

        var request = options.ToRequest();

        Assert.Equal("map.bsp", request.BspPath);
        Assert.Equal("tchinese", request.OutLang);
        Assert.Equal("prompt.md", request.PromptFilePath);
        Assert.Equal("gpt-test", request.LLM.Model);
    }
}
