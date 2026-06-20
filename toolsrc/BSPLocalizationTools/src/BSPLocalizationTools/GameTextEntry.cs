namespace BSPLocalizationTools;

public sealed record GameTextEntry(int Index, string Message);

public interface IGameTextExtractor
{
    IReadOnlyList<GameTextEntry> Extract(string bspPath);
}
