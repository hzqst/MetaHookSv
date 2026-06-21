using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Markup.Xaml;
using BSPLocalizationTools.GUI.ViewModels;
using BSPLocalizationTools.GUI.Views;

namespace BSPLocalizationTools.GUI;

public sealed partial class App : Application
{
    public override void Initialize()
    {
        AvaloniaXamlLoader.Load(this);
    }

    public override void OnFrameworkInitializationCompleted()
    {
        if (ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
        {
            desktop.MainWindow = new MainWindow
            {
                DataContext = MainWindowViewModel.CreateDefault(),
            };
        }

        base.OnFrameworkInitializationCompleted();
    }
}
