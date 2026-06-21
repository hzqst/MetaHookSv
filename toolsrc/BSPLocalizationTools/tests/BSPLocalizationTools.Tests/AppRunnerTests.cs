using System.Text;

namespace BSPLocalizationTools.Tests;

public sealed class AppRunnerTests
{
    [Fact]
    public async Task RunWritesDictionaryNextToBsp()
    {
        using var temp = new TempDirectory();
        var bspPath = Path.Combine(temp.Path, "fake_map.bsp");
        File.WriteAllText(bspPath, "placeholder");

        var runner = new AppRunner(
            new FakeExtractor([
                new GameTextEntry(0, "kinnkyuu jitai da!!\\n(We got a situation!!)"),
                new GameTextEntry(1, "kinnkyuu jitai da!!\\n(We got a situation!!)"),
            ]),
            new FakeLLMClient(),
            new DictionaryCsvWriter());

        var output = await runner.RunAsync(
            new CommandLineOptions(
                bspPath,
                "schinese",
                null,
                new LLMOptions("gpt-5.4", "sk-", null, null, "high", null)),
            CancellationToken.None);

        Assert.Equal(Path.Combine(temp.Path, "fake_map_dictionary_schinese.csv"), output);
        var text = Encoding.UTF8.GetString(File.ReadAllBytes(output));
        Assert.Contains("NETMESSAGE:kinnkyuu jitai da!!\\n(We got a situation!!),緊急事態だ!!\\n（出大事儿了！！）", text);
        Assert.Equal(3, text.Split(Environment.NewLine, StringSplitOptions.RemoveEmptyEntries).Length);
    }

    private sealed class FakeExtractor(IReadOnlyList<GameTextEntry> entries) : IGameTextExtractor
    {
        public IReadOnlyList<GameTextEntry> Extract(string bspPath) => entries;
    }

    private sealed class FakeLLMClient : ILLMClient
    {
        public Task<string> CompleteTextAsync(
            IReadOnlyList<LLMMessage> messages,
            LLMOptions options,
            CancellationToken cancellationToken)
        {
            return Task.FromResult(
                """{"translations":[{"id":0,"translation":"緊急事態だ!!\\n（出大事儿了！！）"}]}""");
        }
    }
}
