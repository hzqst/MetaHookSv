using Avalonia.Controls;
using Avalonia.Input;
using System.Diagnostics;

namespace MetahookInstallerAvalonia.Views;

public partial class MainWindow : Window
{

    public MainWindow()
    {
        InitializeComponent();
    }

    private void TitleBar_PointerPressed(object? sender, PointerPressedEventArgs e)
    {
        if (e.GetCurrentPoint(this).Properties.IsLeftButtonPressed)
        {
            BeginMoveDrag(e);
        }
    }

    private void MinimizeButton_Click(object? sender, Avalonia.Interactivity.RoutedEventArgs e)
    {
        WindowState = WindowState.Minimized;
    }

    private void CloseButton_Click(object? sender, Avalonia.Interactivity.RoutedEventArgs e)
    {
        Close();
    }

    private void GithubButton_Click(object? sender, Avalonia.Interactivity.RoutedEventArgs e)
    {
        Process.Start(new ProcessStartInfo("https://github.com/hzqst/MetaHookSv") { UseShellExecute = true });
    }
}
