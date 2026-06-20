using System.Text;

namespace BSPLocalizationTools;

public sealed record DictionaryRow(string Title, string Translation);

public sealed class DictionaryCsvWriter
{
    private const string Header = "Title,Translation,Color,Duration,Speaker,Nextsubtitle,Delaytonextsubtitle,Style";

    public void Write(string outputPath, string language, IReadOnlyList<DictionaryRow> rows)
    {
        var encoding = LanguageEncoding.Resolve(language);
        var directory = Path.GetDirectoryName(outputPath);
        if (!string.IsNullOrEmpty(directory))
        {
            Directory.CreateDirectory(directory);
        }

        using var writer = new StreamWriter(outputPath, append: false, encoding);
        writer.WriteLine(Header);
        foreach (var row in rows)
        {
            writer.Write(Escape(row.Title));
            writer.Write(',');
            writer.Write(Escape(row.Translation));
            writer.WriteLine(",,,,,,");
        }
    }

    private static string Escape(string value)
    {
        if (value.IndexOfAny([',', '"', '\r', '\n']) < 0)
        {
            return value;
        }

        return "\"" + value.Replace("\"", "\"\"", StringComparison.Ordinal) + "\"";
    }
}
