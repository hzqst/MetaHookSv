using System.Text;

namespace BSPLocalizationTools.Tests;

public sealed class DictionaryCsvWriterTests
{
    [Fact]
    public void SchineseUsesUtf8BomEncoding()
    {
        using var temp = new TempDirectory();
        var path = Path.Combine(temp.Path, "map_dictionary_schinese.csv");
        var writer = new DictionaryCsvWriter();

        writer.Write(path, "schinese", [new DictionaryRow("NETMESSAGE:a", "出大事儿了！！")]);

        var bytes = File.ReadAllBytes(path);
        Assert.True(StartsWithUtf8Bom(bytes));
        var text = Encoding.UTF8.GetString(bytes);
        Assert.Contains("Title,Translation,Color,Duration,Speaker,Nextsubtitle,Delaytonextsubtitle,Style", text);
        Assert.Contains("NETMESSAGE:a,出大事儿了！！", text);
    }

    [Fact]
    public void TchineseUsesUtf8BomEncoding()
    {
        using var temp = new TempDirectory();
        var path = Path.Combine(temp.Path, "map_dictionary_tchinese.csv");
        var writer = new DictionaryCsvWriter();

        writer.Write(path, "tchinese", [new DictionaryRow("NETMESSAGE:a", "繁體中文")]);

        var bytes = File.ReadAllBytes(path);
        Assert.True(StartsWithUtf8Bom(bytes));
        var text = Encoding.UTF8.GetString(bytes);
        Assert.Contains("繁體中文", text);
    }

    [Fact]
    public void CsvEscapesCommaQuoteAndNewline()
    {
        using var temp = new TempDirectory();
        var path = Path.Combine(temp.Path, "map_dictionary_schinese.csv");
        var writer = new DictionaryCsvWriter();

        writer.Write(path, "schinese", [new DictionaryRow("NETMESSAGE:a,b", "say \"hello\"\nnow")]);

        var bytes = File.ReadAllBytes(path);
        Assert.True(StartsWithUtf8Bom(bytes));
        var text = Encoding.UTF8.GetString(bytes);
        Assert.Contains("\"NETMESSAGE:a,b\",\"say \"\"hello\"\"\nnow\"", text);
    }

    private static bool StartsWithUtf8Bom(byte[] bytes)
    {
        return bytes.Length >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF;
    }
}
