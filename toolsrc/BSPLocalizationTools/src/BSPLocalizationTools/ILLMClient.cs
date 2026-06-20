namespace BSPLocalizationTools;

public interface ILLMClient
{
    Task<string> CompleteTextAsync(
        IReadOnlyList<LLMMessage> messages,
        LLMOptions options,
        CancellationToken cancellationToken);
}
