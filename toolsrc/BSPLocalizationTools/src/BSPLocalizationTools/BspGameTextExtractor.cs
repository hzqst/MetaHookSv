using Sledge.Formats.Bsp;
using Sledge.Formats.Bsp.Lumps;

namespace BSPLocalizationTools;

public sealed class BspGameTextExtractor : IGameTextExtractor
{
    public IReadOnlyList<GameTextEntry> Extract(string bspPath)
    {
        using var stream = File.OpenRead(bspPath);
        var bsp = new BspFile(stream);
        var entities = bsp.GetLump<Entities>()
            ?? throw new InvalidOperationException("BSP does not contain an entities lump.");

        var results = new List<GameTextEntry>();
        foreach (var entity in entities)
        {
            if (!string.Equals(entity.ClassName, "game_text", StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            var message = entity.Get("message", "");
            if (string.IsNullOrWhiteSpace(message))
            {
                continue;
            }

            results.Add(new GameTextEntry(results.Count, message));
        }

        return results;
    }
}
