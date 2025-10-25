using ReactiveUI;

namespace MetahookInstallerAvalonia.ViewModels;

public class PluginInfo(string name, bool enabled) : ViewModelBase()
{
    private readonly string _name = name;
    private bool _enabled = enabled;
    private int _index;

    public int Index
    {
        get => _index;
        set => this.RaiseAndSetIfChanged(ref _index, value);
    }

    public string Name => _name;

    public bool Enabled
    {
        get => _enabled;
        set => this.RaiseAndSetIfChanged(ref _enabled, value);
    }

    public PluginInfo Clone()
    {
        return new PluginInfo(_name, _enabled);
    }
}