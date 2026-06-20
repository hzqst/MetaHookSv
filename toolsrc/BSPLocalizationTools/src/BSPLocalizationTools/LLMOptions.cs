namespace BSPLocalizationTools;

public sealed record LLMOptions(
    string? Model,
    string? ApiKey,
    string? BaseUrl,
    double? Temperature,
    string Effort,
    string? FakeAs);
