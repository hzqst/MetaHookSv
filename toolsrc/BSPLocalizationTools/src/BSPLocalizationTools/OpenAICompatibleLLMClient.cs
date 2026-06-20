using System.Net.Http.Headers;
using System.Text;
using System.Text.Json;

namespace BSPLocalizationTools;

public sealed class OpenAICompatibleLLMClient(HttpClient httpClient) : ILLMClient
{
    private const string DefaultBaseUrl = "https://api.openai.com/v1";

    public async Task<string> CompleteTextAsync(
        IReadOnlyList<LLMMessage> messages,
        LLMOptions options,
        CancellationToken cancellationToken)
    {
        ValidateOptions(options);
        return string.Equals(options.FakeAs, "codex", StringComparison.OrdinalIgnoreCase)
            ? await CompleteViaResponsesAsync(messages, options, cancellationToken)
            : await CompleteViaChatAsync(messages, options, cancellationToken);
    }

    private static void ValidateOptions(LLMOptions options)
    {
        if (string.IsNullOrWhiteSpace(options.Model))
        {
            throw new InvalidOperationException("LLM model is required. Use -llm_model or BSPL10N_LLM_MODEL.");
        }

        if (string.IsNullOrWhiteSpace(options.ApiKey))
        {
            throw new InvalidOperationException("LLM API key is required. Use -llm_apikey or BSPL10N_LLM_APIKEY.");
        }

        if (string.Equals(options.FakeAs, "codex", StringComparison.OrdinalIgnoreCase) &&
            string.IsNullOrWhiteSpace(options.BaseUrl))
        {
            throw new InvalidOperationException("-llm_baseurl is required when -llm_fake_as=codex.");
        }
    }

    private async Task<string> CompleteViaChatAsync(
        IReadOnlyList<LLMMessage> messages,
        LLMOptions options,
        CancellationToken cancellationToken)
    {
        var body = new Dictionary<string, object?>
        {
            ["model"] = options.Model,
            ["messages"] = messages.Select(m => new { role = m.Role, content = m.Content }).ToArray(),
            ["reasoning_effort"] = options.Effort,
        };
        if (options.Temperature is not null)
        {
            body["temperature"] = options.Temperature;
        }

        using var request = CreateJsonRequest(GetBaseUrl(options) + "/chat/completions", options, body);
        using var response = await httpClient.SendAsync(request, cancellationToken);
        response.EnsureSuccessStatusCode();
        var json = await response.Content.ReadAsStringAsync(cancellationToken);
        using var document = JsonDocument.Parse(json);
        var content = document.RootElement
            .GetProperty("choices")[0]
            .GetProperty("message")
            .GetProperty("content")
            .GetString();
        return string.IsNullOrWhiteSpace(content)
            ? throw new InvalidOperationException("LLM chat response was empty.")
            : content;
    }

    private async Task<string> CompleteViaResponsesAsync(
        IReadOnlyList<LLMMessage> messages,
        LLMOptions options,
        CancellationToken cancellationToken)
    {
        var body = new Dictionary<string, object?>
        {
            ["model"] = options.Model,
            ["input"] = new[]
            {
                new { role = "user", content = string.Join("\n\n", messages.Select(m => m.Content)) },
            },
            ["reasoning"] = new { effort = options.Effort },
            ["stream"] = true,
        };
        if (options.Temperature is not null)
        {
            body["temperature"] = options.Temperature;
        }

        using var request = CreateJsonRequest(GetBaseUrl(options) + "/responses", options, body);
        request.Headers.Accept.Clear();
        request.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("text/event-stream"));
        using var response = await httpClient.SendAsync(request, HttpCompletionOption.ResponseHeadersRead, cancellationToken);
        response.EnsureSuccessStatusCode();
        var text = await response.Content.ReadAsStringAsync(cancellationToken);
        var result = ExtractSseText(text);
        return string.IsNullOrWhiteSpace(result)
            ? throw new InvalidOperationException("LLM responses stream was empty.")
            : result;
    }

    private static HttpRequestMessage CreateJsonRequest(string uri, LLMOptions options, object body)
    {
        var request = new HttpRequestMessage(HttpMethod.Post, uri);
        request.Headers.Authorization = new AuthenticationHeaderValue("Bearer", options.ApiKey);
        request.Headers.UserAgent.ParseAdd("BSPLocalizationTools/1.0");
        request.Content = new StringContent(JsonSerializer.Serialize(body), Encoding.UTF8, "application/json");
        return request;
    }

    private static string GetBaseUrl(LLMOptions options)
    {
        return (string.IsNullOrWhiteSpace(options.BaseUrl) ? DefaultBaseUrl : options.BaseUrl.Trim()).TrimEnd('/');
    }

    private static string ExtractSseText(string text)
    {
        var builder = new StringBuilder();
        foreach (var rawLine in text.Split('\n'))
        {
            var line = rawLine.Trim();
            if (!line.StartsWith("data:", StringComparison.Ordinal))
            {
                continue;
            }

            var payload = line[5..].Trim();
            if (payload == "[DONE]")
            {
                break;
            }

            using var document = JsonDocument.Parse(payload);
            var root = document.RootElement;
            if (root.TryGetProperty("type", out var type) &&
                type.GetString() == "response.output_text.delta" &&
                root.TryGetProperty("delta", out var delta))
            {
                builder.Append(delta.GetString());
            }
        }

        return builder.ToString();
    }
}
