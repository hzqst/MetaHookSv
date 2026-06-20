using System.Text;

namespace BSPLocalizationTools;

public static class LanguageEncoding
{
    public static Encoding Resolve(string language)
    {
        Encoding.RegisterProvider(CodePagesEncodingProvider.Instance);
        return language.Trim().ToLowerInvariant() switch
        {
            "schinese" => Encoding.GetEncoding(936),
            "tchinese" => Encoding.GetEncoding(950),
            _ => new UTF8Encoding(encoderShouldEmitUTF8Identifier: false),
        };
    }
}
