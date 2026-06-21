using System.Text;

namespace BSPLocalizationTools;

public static class LanguageEncoding
{
    public static Encoding Resolve(string language)
    {
        return new UTF8Encoding(encoderShouldEmitUTF8Identifier: true);
    }
}
