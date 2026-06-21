using System.Collections.ObjectModel;
using System.Globalization;
using System.Text;
using System.Windows.Input;
using BSPLocalizationTools.GUI.Lang;
using ReactiveUI;

namespace BSPLocalizationTools.GUI.ViewModels;

public sealed partial class MainWindowViewModel : ViewModelBase
{
    private readonly BatchLocalizationRunner _batchRunner;
    private readonly IGameTextExtractor _gameTextExtractor;
    private readonly GuiLocalizer _localizer;
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
    private bool _appendLanguageToCsvFileName;
    private double _overallProgress;
    private bool _isTranslating;
    private string _logText = "";
    private LocalizedStrings _strings;
    private GuiLanguageOption _selectedGuiLanguage;

    public MainWindowViewModel(
        BatchLocalizationRunner batchRunner,
        string envPath,
        Func<CultureInfo>? systemCultureProvider = null,
        IGameTextExtractor? gameTextExtractor = null)
    {
        _batchRunner = batchRunner;
        _gameTextExtractor = gameTextExtractor ?? new BspGameTextExtractor();
        _localizer = new GuiLocalizer(systemCultureProvider);
        _envPath = envPath;
        _outLang = ToolConfiguration.Default.DefaultOutLang;
        _llmEffort = ToolConfiguration.Default.LLM.Effort;
        _appendLanguageToCsvFileName = ToolConfiguration.Default.AppendLanguageToCsvFileName;
        _localizer.SetLanguage(ToolConfiguration.Default.GuiLanguage);
        _strings = LocalizedStrings.Current();
        _selectedGuiLanguage = CreateLanguageOptions().First(o => o.Code == ToolConfiguration.Default.GuiLanguage);
        RefreshLanguageOptions();
        LoadConfigurationCommand = ReactiveCommand.Create(LoadConfiguration);
        SaveConfigurationCommand = ReactiveCommand.Create(SaveConfiguration);
        LoadBuiltInPromptCommand = ReactiveCommand.Create(LoadBuiltInPrompt);
        ReloadPromptFromFileCommand = ReactiveCommand.Create(ReloadPromptFromFile);
        SavePromptCommand = ReactiveCommand.Create(SavePrompt);
        RemoveItemCommand = ReactiveCommand.Create<TranslationItemViewModel>(RemoveItem, this.WhenAnyValue(
            x => x.IsTranslating,
            isTranslating => !isTranslating));
        ClearCompletedItemsCommand = ReactiveCommand.Create(ClearCompletedItems, this.WhenAnyValue(
            x => x.IsTranslating,
            isTranslating => !isTranslating));
        ClearAllItemsCommand = ReactiveCommand.Create(ClearAllItems, this.WhenAnyValue(
            x => x.IsTranslating,
            isTranslating => !isTranslating));
        StartTranslationCommand = ReactiveCommand.CreateFromTask(StartTranslationAsync, this.WhenAnyValue(
            x => x.IsTranslating,
            isTranslating => !isTranslating));
        CancelTranslationCommand = ReactiveCommand.Create(CancelTranslation, this.WhenAnyValue(
            x => x.IsTranslating));
    }

    public ObservableCollection<TranslationItemViewModel> Items { get; } = [];
    public IReadOnlyList<string> EffortOptions { get; } = ["minimal", "low", "medium", "high"];
    public ObservableCollection<GuiLanguageOption> GuiLanguageOptions { get; } = [];
    public ICommand LoadConfigurationCommand { get; }
    public ICommand SaveConfigurationCommand { get; }
    public ICommand LoadBuiltInPromptCommand { get; }
    public ICommand ReloadPromptFromFileCommand { get; }
    public ICommand SavePromptCommand { get; }
    public ICommand RemoveItemCommand { get; }
    public ICommand ClearCompletedItemsCommand { get; }
    public ICommand ClearAllItemsCommand { get; }
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

    public bool AppendLanguageToCsvFileName
    {
        get => _appendLanguageToCsvFileName;
        set => this.RaiseAndSetIfChanged(ref _appendLanguageToCsvFileName, value);
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

    public LocalizedStrings Strings
    {
        get => _strings;
        private set => this.RaiseAndSetIfChanged(ref _strings, value);
    }

    public GuiLanguageOption SelectedGuiLanguage
    {
        get => _selectedGuiLanguage;
        set
        {
            if (value == _selectedGuiLanguage)
            {
                return;
            }

            this.RaiseAndSetIfChanged(ref _selectedGuiLanguage, value);
            ApplyGuiLanguage(value.Code);
        }
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

    public void RemoveItem(TranslationItemViewModel? item)
    {
        if (item is not null && !IsTranslating)
        {
            Items.Remove(item);
        }
    }

    public void ClearCompletedItems()
    {
        if (IsTranslating)
        {
            return;
        }

        foreach (var item in Items.Where(i => i.Stage == TranslationStage.Completed).ToArray())
        {
            Items.Remove(item);
        }
    }

    public void ClearAllItems()
    {
        if (!IsTranslating)
        {
            Items.Clear();
        }
    }

    public string GetRawGameText(TranslationItemViewModel item)
    {
        try
        {
            var entries = _gameTextExtractor.Extract(item.BspPath);
            if (entries.Count == 0)
            {
                return Strings.NoGameTextFound;
            }

            return string.Join(
                Environment.NewLine,
                entries.Select(entry => string.Create(
                    CultureInfo.InvariantCulture,
                    $"[{entry.Index}] {entry.Message}")));
        }
        catch (Exception ex)
        {
            return string.Format(CultureInfo.CurrentCulture, Strings.FailedToLoadGameText, ex.Message);
        }
    }

    public void LoadConfiguration()
    {
        ApplyConfiguration(ToolConfigurationFile.Load(EnvPath));
        LoadPromptFromConfiguredPath(logSuccess: false);
        AppendLog(string.Format(CultureInfo.CurrentCulture, Resources.LogLoadedConfiguration, EnvPath));
    }

    public void SaveConfiguration()
    {
        ToolConfigurationFile.Save(EnvPath, CreateConfiguration());
        AppendLog(string.Format(CultureInfo.CurrentCulture, Resources.LogSavedConfiguration, EnvPath));
    }

    public async Task StartTranslationAsync()
    {
        if (Items.Count == 0)
        {
            AppendLog(Resources.LogNoBspFilesSelected);
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
                CreateConfiguration().LLM,
                AppendLanguageToCsvFileName)).ToArray();
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
        AppendLog(Resources.LogCancelRequested);
    }

    private void ApplyConfiguration(ToolConfiguration configuration)
    {
        SelectedGuiLanguage = FindLanguageOption(configuration.GuiLanguage);
        OutLang = configuration.DefaultOutLang;
        PromptFilePath = configuration.DefaultPromptFilePath;
        LlmModel = configuration.LLM.Model;
        LlmApiKey = configuration.LLM.ApiKey;
        LlmBaseUrl = configuration.LLM.BaseUrl;
        LlmTemperature = configuration.LLM.Temperature?.ToString(CultureInfo.InvariantCulture);
        LlmEffort = configuration.LLM.Effort;
        LlmFakeAs = configuration.LLM.FakeAs;
        AppendLanguageToCsvFileName = configuration.AppendLanguageToCsvFileName;
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
            Normalize(PromptFilePath),
            SelectedGuiLanguage.Code,
            AppendLanguageToCsvFileName);
    }

    private void ResetItems()
    {
        foreach (var item in Items)
        {
            item.Stage = TranslationStage.Queued;
            item.Status = _localizer.GetStageText(TranslationStage.Queued);
            item.OutputPath = null;
            item.ErrorMessage = null;
        }
    }

    private void HandleProgress(TranslationProgress progress)
    {
        var item = Items.FirstOrDefault(i => string.Equals(i.BspPath, progress.BspPath, StringComparison.OrdinalIgnoreCase));
        if (item is not null)
        {
            item.Stage = progress.Stage;
            item.Status = _localizer.GetStageText(progress.Stage);
        }

        OverallProgress = progress.ItemCount == 0
            ? 0
            : Math.Clamp((double)progress.ItemIndex / progress.ItemCount, 0, 1);
        AppendLog(string.Format(
            CultureInfo.CurrentCulture,
            Resources.LogProgress,
            Path.GetFileName(progress.BspPath),
            progress.Message));
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
            item.Stage = result.Skipped
                ? TranslationStage.Skipped
                : result.Succeeded
                    ? TranslationStage.Completed
                    : TranslationStage.Failed;
            item.Status = _localizer.GetStageText(item.Stage);
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

    private void ApplyGuiLanguage(string languageCode)
    {
        _localizer.SetLanguage(languageCode);
        RefreshLocalizedStrings();
    }

    private void RefreshLocalizedStrings()
    {
        Strings = LocalizedStrings.Current();
        RefreshLanguageOptions();
        foreach (var item in Items)
        {
            item.Status = _localizer.GetStageText(item.Stage);
        }
    }

    private void RefreshLanguageOptions()
    {
        var selectedCode = _selectedGuiLanguage.Code;
        GuiLanguageOptions.Clear();
        foreach (var option in CreateLanguageOptions())
        {
            GuiLanguageOptions.Add(option);
            if (option.Code == selectedCode)
            {
                _selectedGuiLanguage = option;
                this.RaisePropertyChanged(nameof(SelectedGuiLanguage));
            }
        }
    }

    private GuiLanguageOption FindLanguageOption(string? code)
    {
        var normalized = GuiLocalizer.NormalizeConfiguredLanguage(code);
        return GuiLanguageOptions.FirstOrDefault(o => o.Code == normalized)
            ?? CreateLanguageOptions().First(o => o.Code == normalized);
    }

    private static IReadOnlyList<GuiLanguageOption> CreateLanguageOptions()
    {
        return
        [
            new GuiLanguageOption(GuiLocalizer.AutoLanguageCode, Resources.LanguageAuto),
            new GuiLanguageOption(GuiLocalizer.EnglishLanguageCode, Resources.LanguageEnglish),
            new GuiLanguageOption(GuiLocalizer.SimplifiedChineseLanguageCode, Resources.LanguageSimplifiedChinese),
            new GuiLanguageOption(GuiLocalizer.TraditionalChineseLanguageCode, Resources.LanguageTraditionalChinese),
        ];
    }
}
