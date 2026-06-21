using BSPLocalizationTools;
using BSPLocalizationTools.CLI;

try
{
    EnvironmentFileLoader.LoadFromCurrentDirectory();

    var options = CommandLineOptions.Parse(args, Environment.GetEnvironmentVariable);
    using var httpClient = new HttpClient
    {
        Timeout = TimeSpan.FromMinutes(5),
    };
    var runner = new LocalizationRunner(
        new BspGameTextExtractor(),
        new OpenAICompatibleLLMClient(httpClient),
        new DictionaryCsvWriter());

    var output = await runner.RunAsync(options.ToRequest(), null, CancellationToken.None);
    Console.WriteLine(output.Skipped
        ? "Skipped: no game_text messages were found."
        : $"Wrote dictionary: {output.OutputPath}");
    return 0;
}
catch (Exception ex)
{
    Console.Error.WriteLine("Error: " + ex.Message);
    return 1;
}
