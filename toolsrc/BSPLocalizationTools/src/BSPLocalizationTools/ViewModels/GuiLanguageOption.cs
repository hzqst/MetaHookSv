using ReactiveUI;

namespace BSPLocalizationTools.GUI.ViewModels;

public sealed class GuiLanguageOption(string code, string displayName) : ViewModelBase
{
    private string _displayName = displayName;

    public string Code { get; } = code;

    public string DisplayName
    {
        get => _displayName;
        set => this.RaiseAndSetIfChanged(ref _displayName, value);
    }
}
