using System.Globalization;
using System.Text;
using CsvHelper;
using CsvHelper.Configuration;

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
        using var csv = new CsvWriter(writer, new CsvConfiguration(CultureInfo.InvariantCulture));
        WriteHeader(csv);
        foreach (var row in rows)
        {
            csv.WriteField(row.Title);
            csv.WriteField(row.Translation);
            for (var i = 0; i < 6; i++)
            {
                csv.WriteField(string.Empty);
            }

            csv.NextRecord();
        }
    }

    private static void WriteHeader(CsvWriter csv)
    {
        foreach (var header in Header.Split(','))
        {
            csv.WriteField(header);
        }

        csv.NextRecord();
    }
}
