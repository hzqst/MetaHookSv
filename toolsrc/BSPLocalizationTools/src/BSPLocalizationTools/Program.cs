using BSPLocalizationTools;

try
{
    EnvironmentFileLoader.LoadFromCurrentDirectory();

    var options = CommandLineOptions.Parse(args, Environment.GetEnvironmentVariable);
    using var httpClient = new HttpClient
    {
        Timeout = TimeSpan.FromMinutes(5),
    };
    var runner = new AppRunner(
        new BspGameTextExtractor(),
        new OpenAICompatibleLLMClient(httpClient),
        new DictionaryCsvWriter());

    var output = await runner.RunAsync(options, CancellationToken.None);
    Console.WriteLine($"Wrote dictionary: {output}");
    return 0;
}
catch (Exception ex)
{
    Console.Error.WriteLine("Error: " + ex.Message);
    return 1;
}
