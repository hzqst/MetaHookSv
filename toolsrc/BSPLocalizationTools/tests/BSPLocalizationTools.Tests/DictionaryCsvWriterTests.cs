using System.Text;

namespace BSPLocalizationTools.Tests;

public sealed class DictionaryCsvWriterTests
{
    [Fact]
    public void SchineseUsesGbkEncodingWithoutBom()
    {
        using var temp = new TempDirectory();
        var path = Path.Combine(temp.Path, "map_dictionary_schinese.csv");
        var writer = new DictionaryCsvWriter();

        writer.Write(path, "schinese", [new DictionaryRow("NETMESSAGE:a", "出大事儿了！！")]);

        var bytes = File.ReadAllBytes(path);
        Assert.False(bytes.Length >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF);
        var text = Encoding.GetEncoding(936).GetString(bytes);
        Assert.Contains("Title,Translation,Color,Duration,Speaker,Nextsubtitle,Delaytonextsubtitle,Style", text);
        Assert.Contains("NETMESSAGE:a,出大事儿了！！", text);
    }

    [Fact]
    public void TchineseUsesBig5Encoding()
    {
        using var temp = new TempDirectory();
        var path = Path.Combine(temp.Path, "map_dictionary_tchinese.csv");
        var writer = new DictionaryCsvWriter();

        writer.Write(path, "tchinese", [new DictionaryRow("NETMESSAGE:a", "繁體中文")]);

        var text = Encoding.GetEncoding(950).GetString(File.ReadAllBytes(path));
        Assert.Contains("繁體中文", text);
    }

    [Fact]
    public void CsvEscapesCommaQuoteAndNewline()
    {
        using var temp = new TempDirectory();
        var path = Path.Combine(temp.Path, "map_dictionary_schinese.csv");
        var writer = new DictionaryCsvWriter();

        writer.Write(path, "schinese", [new DictionaryRow("NETMESSAGE:a,b", "say \"hello\"\nnow")]);

        var text = Encoding.GetEncoding(936).GetString(File.ReadAllBytes(path));
        Assert.Contains("\"NETMESSAGE:a,b\",\"say \"\"hello\"\"\nnow\"", text);
    }
}
