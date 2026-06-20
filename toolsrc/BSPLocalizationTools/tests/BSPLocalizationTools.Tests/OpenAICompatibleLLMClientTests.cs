using System.Net;
using System.Text;

namespace BSPLocalizationTools.Tests;

public sealed class OpenAICompatibleLLMClientTests
{
    [Fact]
    public async Task ChatCompletionsReturnsFirstMessageText()
    {
        var handler = new FakeHttpHandler(_ => new HttpResponseMessage(HttpStatusCode.OK)
        {
            Content = new StringContent(
                """{"choices":[{"message":{"content":"{\"translations\":[{\"id\":0,\"translation\":\"你好\"}]}"}}]}""",
                Encoding.UTF8,
                "application/json"),
        });
        var client = new OpenAICompatibleLLMClient(new HttpClient(handler));

        var result = await client.CompleteTextAsync(
            [new LLMMessage("user", "hello")],
            new LLMOptions("gpt-test", "key", "https://example.test/v1", null, "medium", null),
            CancellationToken.None);

        Assert.Contains("translations", result);
        Assert.Equal("https://example.test/v1/chat/completions", handler.RequestUri!.ToString());
    }

    [Fact]
    public async Task CodexResponsesReadsSseDelta()
    {
        var handler = new FakeHttpHandler(_ => new HttpResponseMessage(HttpStatusCode.OK)
        {
            Content = new StringContent(
                "data: {\"type\":\"response.output_text.delta\",\"delta\":\"hello\"}\n\ndata: [DONE]\n\n",
                Encoding.UTF8,
                "text/event-stream"),
        });
        var client = new OpenAICompatibleLLMClient(new HttpClient(handler));

        var result = await client.CompleteTextAsync(
            [new LLMMessage("user", "hello")],
            new LLMOptions("gpt-test", "key", "https://example.test/v1", null, "medium", "codex"),
            CancellationToken.None);

        Assert.Equal("hello", result);
        Assert.Equal("https://example.test/v1/responses", handler.RequestUri!.ToString());
    }

    [Fact]
    public async Task CodexResponsesIncludesSystemAndUserMessages()
    {
        var handler = new FakeHttpHandler(_ => new HttpResponseMessage(HttpStatusCode.OK)
        {
            Content = new StringContent(
                "data: {\"type\":\"response.output_text.delta\",\"delta\":\"ok\"}\n\ndata: [DONE]\n\n",
                Encoding.UTF8,
                "text/event-stream"),
        });
        var client = new OpenAICompatibleLLMClient(new HttpClient(handler));

        await client.CompleteTextAsync(
            [
                new LLMMessage("system", "custom translation prompt"),
                new LLMMessage("user", "{\"inputs\":[]}"),
            ],
            new LLMOptions("gpt-test", "key", "https://example.test/v1", null, "medium", "codex"),
            CancellationToken.None);

        Assert.Contains("custom translation prompt", handler.RequestBody);
        Assert.Contains("{\\u0022inputs\\u0022:[]}", handler.RequestBody);
        Assert.Contains(@"""role"":""user""", handler.RequestBody);
        Assert.DoesNotContain(@"""role"":""system""", handler.RequestBody);
    }

    private sealed class FakeHttpHandler(Func<HttpRequestMessage, HttpResponseMessage> respond) : HttpMessageHandler
    {
        public Uri? RequestUri { get; private set; }
        public string RequestBody { get; private set; } = "";

        protected override async Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
        {
            RequestUri = request.RequestUri;
            if (request.Content is not null)
            {
                RequestBody = await request.Content.ReadAsStringAsync(cancellationToken);
            }

            return respond(request);
        }
    }
}
