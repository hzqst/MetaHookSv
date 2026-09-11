using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Markup.Xaml;
using Avalonia.Styling;
using MetahookInstallerAvalonia.ViewModels;
using MetahookInstallerAvalonia.Views;
using System;
using System.Globalization;
using System.IO;

namespace MetahookInstallerAvalonia;

public partial class App : Application
{
    public override void Initialize()
    {
        AvaloniaXamlLoader.Load(this);
    }

    public override void OnFrameworkInitializationCompleted()
    {
        var settingPath = Path.Combine(".", "lang");
        if (File.Exists(settingPath))
        {
            string lang = File.ReadAllText(settingPath);
            Lang.Resources.Culture = new CultureInfo(lang);
        }
        var themePath = Path.Combine(".", "theme");
        if (File.Exists(themePath))
        {
            RequestedThemeVariant = File.ReadAllText(themePath).Trim() switch
            {
                "Light" => ThemeVariant.Light,
                "Dark" => ThemeVariant.Dark,
                _ => ThemeVariant.Default,
            };
        }
        if (ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
        {
            desktop.MainWindow = new MainWindow
            {
                DataContext = new MainViewModel()
            };
        }
        else if (ApplicationLifetime is ISingleViewApplicationLifetime singleViewPlatform)
        {
            singleViewPlatform.MainView = new MainView
            {
                DataContext = new MainViewModel()
            };
        }

        base.OnFrameworkInitializationCompleted();
    }
}
