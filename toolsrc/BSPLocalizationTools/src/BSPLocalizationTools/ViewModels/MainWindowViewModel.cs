using System.Collections.ObjectModel;
using System.Globalization;
using System.Text;
using System.Windows.Input;
using ReactiveUI;

namespace BSPLocalizationTools.GUI.ViewModels;

public sealed class MainWindowViewModel : ViewModelBase
{
    private readonly BatchLocalizationRunner _batchRunner;
    private CancellationTokenSource? _translationCancellation;
    private string _outLang;
    private string? _promptFilePath;
    private string? _llmModel;
    private string? _llmApiKey;
    private string? _llmBaseUrl;
    private string? _llmTemperature;
    private string _llmEffort;
    private string? _llmFakeAs;
    private string _envPath;
    private double _overallProgress;
    private bool _isTranslating;
    private string _logText = "";

    public MainWindowViewModel(BatchLocalizationRunner batchRunner, string envPath)
    {
        _batchRunner = batchRunner;
        _envPath = envPath;
        _outLang = ToolConfiguration.Default.DefaultOutLang;
        _llmEffort = ToolConfiguration.Default.LLM.Effort;
        LoadConfigurationCommand = ReactiveCommand.Create(LoadConfiguration);
        SaveConfigurationCommand = ReactiveCommand.Create(SaveConfiguration);
        StartTranslationCommand = ReactiveCommand.CreateFromTask(StartTranslationAsync, this.WhenAnyValue(
            x => x.IsTranslating,
            isTranslating => !isTranslating));
        CancelTranslationCommand = ReactiveCommand.Create(CancelTranslation, this.WhenAnyValue(
            x => x.IsTranslating));
    }

    public ObservableCollection<TranslationItemViewModel> Items { get; } = [];
    public IReadOnlyList<string> EffortOptions { get; } = ["minimal", "low", "medium", "high"];
    public ICommand LoadConfigurationCommand { get; }
    public ICommand SaveConfigurationCommand { get; }
    public ICommand StartTranslationCommand { get; }
    public ICommand CancelTranslationCommand { get; }

    public string EnvPath
    {
        get => _envPath;
        set => this.RaiseAndSetIfChanged(ref _envPath, value);
    }

    public string OutLang
    {
        get => _outLang;
        set => this.RaiseAndSetIfChanged(ref _outLang, value);
    }

    public string? PromptFilePath
    {
        get => _promptFilePath;
        set => this.RaiseAndSetIfChanged(ref _promptFilePath, value);
    }

    public string? LlmModel
    {
        get => _llmModel;
        set => this.RaiseAndSetIfChanged(ref _llmModel, value);
    }

    public string? LlmApiKey
    {
        get => _llmApiKey;
        set => this.RaiseAndSetIfChanged(ref _llmApiKey, value);
    }

    public string? LlmBaseUrl
    {
        get => _llmBaseUrl;
        set => this.RaiseAndSetIfChanged(ref _llmBaseUrl, value);
    }

    public string? LlmTemperature
    {
        get => _llmTemperature;
        set => this.RaiseAndSetIfChanged(ref _llmTemperature, value);
    }

    public string LlmEffort
    {
        get => _llmEffort;
        set => this.RaiseAndSetIfChanged(ref _llmEffort, value);
    }

    public string? LlmFakeAs
    {
        get => _llmFakeAs;
        set => this.RaiseAndSetIfChanged(ref _llmFakeAs, value);
    }

    public double OverallProgress
    {
        get => _overallProgress;
        set => this.RaiseAndSetIfChanged(ref _overallProgress, value);
    }

    public bool IsTranslating
    {
        get => _isTranslating;
        set => this.RaiseAndSetIfChanged(ref _isTranslating, value);
    }

    public string LogText
    {
        get => _logText;
        set => this.RaiseAndSetIfChanged(ref _logText, value);
    }

    public static MainWindowViewModel CreateDefault()
    {
        var httpClient = new HttpClient
        {
            Timeout = TimeSpan.FromMinutes(5),
        };
        var runner = new LocalizationRunner(
            new BspGameTextExtractor(),
            new OpenAICompatibleLLMClient(httpClient),
            new DictionaryCsvWriter());
        var vm = new MainWindowViewModel(new BatchLocalizationRunner(runner), ToolConfigurationFile.GetDefaultPath());
        vm.LoadConfiguration();
        return vm;
    }

    public void AddBspFiles(IEnumerable<string> bspPaths)
    {
        var existing = Items.Select(i => i.BspPath).ToHashSet(StringComparer.OrdinalIgnoreCase);
        foreach (var path in bspPaths.Where(p => string.Equals(Path.GetExtension(p), ".bsp", StringComparison.OrdinalIgnoreCase)))
        {
            if (existing.Add(path))
            {
                Items.Add(new TranslationItemViewModel(path));
            }
        }
    }

    public void LoadConfiguration()
    {
        ApplyConfiguration(ToolConfigurationFile.Load(EnvPath));
        AppendLog($"Loaded configuration: {EnvPath}");
    }

    public void SaveConfiguration()
    {
        ToolConfigurationFile.Save(EnvPath, CreateConfiguration());
        AppendLog($"Saved configuration: {EnvPath}");
    }

    public async Task StartTranslationAsync()
    {
        if (Items.Count == 0)
        {
            AppendLog("No BSP files selected.");
            return;
        }

        IsTranslating = true;
        OverallProgress = 0;
        _translationCancellation = new CancellationTokenSource();
        ResetItems();

        try
        {
            var requests = Items.Select(item => new LocalizationRequest(
                item.BspPath,
                OutLang,
                Normalize(PromptFilePath),
                CreateConfiguration().LLM)).ToArray();
            var progress = new Progress<TranslationProgress>(HandleProgress);
            var results = await _batchRunner.RunAsync(
                new LocalizationBatchRequest(requests),
                progress,
                _translationCancellation.Token);
            ApplyResults(results);
        }
        finally
        {
            _translationCancellation.Dispose();
            _translationCancellation = null;
            IsTranslating = false;
        }
    }

    public void CancelTranslation()
    {
        _translationCancellation?.Cancel();
        AppendLog("Cancel requested.");
    }

    private void ApplyConfiguration(ToolConfiguration configuration)
    {
        OutLang = configuration.DefaultOutLang;
        PromptFilePath = configuration.DefaultPromptFilePath;
        LlmModel = configuration.LLM.Model;
        LlmApiKey = configuration.LLM.ApiKey;
        LlmBaseUrl = configuration.LLM.BaseUrl;
        LlmTemperature = configuration.LLM.Temperature?.ToString(CultureInfo.InvariantCulture);
        LlmEffort = configuration.LLM.Effort;
        LlmFakeAs = configuration.LLM.FakeAs;
    }

    private ToolConfiguration CreateConfiguration()
    {
        return new ToolConfiguration(
            new LLMOptions(
                Normalize(LlmModel),
                Normalize(LlmApiKey),
                Normalize(LlmBaseUrl),
                ParseTemperature(LlmTemperature),
                string.IsNullOrWhiteSpace(LlmEffort) ? ToolConfiguration.Default.LLM.Effort : LlmEffort.Trim(),
                Normalize(LlmFakeAs)),
            string.IsNullOrWhiteSpace(OutLang) ? ToolConfiguration.Default.DefaultOutLang : OutLang.Trim(),
            Normalize(PromptFilePath));
    }

    private void ResetItems()
    {
        foreach (var item in Items)
        {
            item.Status = "Queued";
            item.OutputPath = null;
            item.ErrorMessage = null;
        }
    }

    private void HandleProgress(TranslationProgress progress)
    {
        var item = Items.FirstOrDefault(i => string.Equals(i.BspPath, progress.BspPath, StringComparison.OrdinalIgnoreCase));
        if (item is not null)
        {
            item.Status = progress.Stage.ToString();
        }

        OverallProgress = progress.ItemCount == 0
            ? 0
            : Math.Clamp((double)progress.ItemIndex / progress.ItemCount, 0, 1);
        AppendLog($"{Path.GetFileName(progress.BspPath)}: {progress.Message}");
    }

    private void ApplyResults(IReadOnlyList<LocalizationBatchItemResult> results)
    {
        foreach (var result in results)
        {
            var item = Items.FirstOrDefault(i => string.Equals(i.BspPath, result.BspPath, StringComparison.OrdinalIgnoreCase));
            if (item is null)
            {
                continue;
            }

            item.OutputPath = result.OutputPath;
            item.ErrorMessage = result.ErrorMessage;
            item.Status = result.Succeeded ? "Completed" : "Failed";
        }
    }

    private void AppendLog(string message)
    {
        var builder = new StringBuilder(LogText);
        if (builder.Length > 0)
        {
            builder.AppendLine();
        }

        builder.Append(message);
        LogText = builder.ToString();
    }

    private static double? ParseTemperature(string? value)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            return null;
        }

        return double.TryParse(value.Trim(), NumberStyles.Float, CultureInfo.InvariantCulture, out var parsed)
            ? parsed
            : throw new InvalidOperationException("LLM temperature must be a number.");
    }

    private static string? Normalize(string? value)
    {
        return string.IsNullOrWhiteSpace(value) ? null : value.Trim();
    }
}
