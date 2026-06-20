namespace BSPLocalizationTools.Tests;

[Collection("Process environment")]
public sealed class EnvironmentFileLoaderTests
{
    [Fact]
    public void LoadFromCurrentDirectoryReadsDotEnvWithoutOverwritingExistingEnvironment()
    {
        using var temp = new TempDirectory();
        var originalCurrentDirectory = Environment.CurrentDirectory;
        var previousModel = Environment.GetEnvironmentVariable("BSPL10N_LLM_MODEL");
        var previousApiKey = Environment.GetEnvironmentVariable("BSPL10N_LLM_APIKEY");

        try
        {
            Environment.SetEnvironmentVariable("BSPL10N_LLM_MODEL", "shell-model");
            Environment.SetEnvironmentVariable("BSPL10N_LLM_APIKEY", null);
            File.WriteAllText(
                Path.Combine(temp.Path, ".env"),
                """
                BSPL10N_LLM_MODEL=dotenv-model
                BSPL10N_LLM_APIKEY=dotenv-key
                """);

            Environment.CurrentDirectory = temp.Path;

            EnvironmentFileLoader.LoadFromCurrentDirectory();

            Assert.Equal("shell-model", Environment.GetEnvironmentVariable("BSPL10N_LLM_MODEL"));
            Assert.Equal("dotenv-key", Environment.GetEnvironmentVariable("BSPL10N_LLM_APIKEY"));
        }
        finally
        {
            Environment.CurrentDirectory = originalCurrentDirectory;
            Environment.SetEnvironmentVariable("BSPL10N_LLM_MODEL", previousModel);
            Environment.SetEnvironmentVariable("BSPL10N_LLM_APIKEY", previousApiKey);
        }
    }
}

[CollectionDefinition("Process environment", DisableParallelization = true)]
public sealed class ProcessEnvironmentCollection;
