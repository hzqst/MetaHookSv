using ReactiveUI;

namespace BSPLocalizationTools.GUI.ViewModels;

public sealed class TranslationItemViewModel(string bspPath) : ViewModelBase
{
    private string _status = "Queued";
    private string? _outputPath;
    private string? _errorMessage;
    private TranslationStage _stage = TranslationStage.Queued;

    public string BspPath { get; } = bspPath;
    public string FileName => Path.GetFileName(BspPath);

    public TranslationStage Stage
    {
        get => _stage;
        set => this.RaiseAndSetIfChanged(ref _stage, value);
    }

    public string Status
    {
        get => _status;
        set => this.RaiseAndSetIfChanged(ref _status, value);
    }

    public string? OutputPath
    {
        get => _outputPath;
        set => this.RaiseAndSetIfChanged(ref _outputPath, value);
    }

    public string? ErrorMessage
    {
        get => _errorMessage;
        set => this.RaiseAndSetIfChanged(ref _errorMessage, value);
    }
}
