namespace MetahookInstallerAvalonia.ViewModels;

public class PluginInfo(string name, bool enabled) : ViewModelBase()
{
    private readonly string _name = name;
    private bool _enabled = enabled;

    public string Name { get => _name; }
    public bool Enabled { get => _enabled; set => _enabled = value; }

    public PluginInfo Clone()
    {
        return new PluginInfo(_name, _enabled);
    }
}
