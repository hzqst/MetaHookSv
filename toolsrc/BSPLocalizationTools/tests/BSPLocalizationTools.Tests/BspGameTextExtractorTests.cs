namespace BSPLocalizationTools.Tests;

public sealed class BspGameTextExtractorTests
{
    [Fact]
    public void ExtractsGameTextMessagesFromPizzaYaSanMap()
    {
        var extractor = new BspGameTextExtractor();

        var entries = extractor.Extract(FindMap("pizza_ya_san1.bsp"));

        Assert.Equal(74, entries.Count);
        Assert.Contains(entries, entry =>
            entry.Message == "kinnkyuu jitai da!!\\n(We got a situation!!)");
        Assert.Contains(entries, entry =>
            entry.Message == "pizza wo nerau bakemono ga kuru zo!!\\n(The monsters tracking down pizza are incoming!!)");
    }

    private static string FindMap(string name)
    {
        var directory = new DirectoryInfo(AppContext.BaseDirectory);
        while (directory is not null)
        {
            var candidate = Path.Combine(directory.FullName, "maps", name);
            if (File.Exists(candidate))
            {
                return candidate;
            }

            directory = directory.Parent;
        }

        throw new FileNotFoundException($"Could not find test map '{name}'.");
    }
}
