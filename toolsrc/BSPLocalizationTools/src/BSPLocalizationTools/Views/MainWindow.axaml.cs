using Avalonia.Controls;
using Avalonia.Platform.Storage;
using BSPLocalizationTools.GUI.ViewModels;

namespace BSPLocalizationTools.GUI.Views;

public sealed partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();
    }

    private async void AddBspButton_Click(object? sender, Avalonia.Interactivity.RoutedEventArgs e)
    {
        var files = await StorageProvider.OpenFilePickerAsync(new FilePickerOpenOptions
        {
            Title = "Select BSP files",
            AllowMultiple = true,
            FileTypeFilter =
            [
                new FilePickerFileType("GoldSrc BSP")
                {
                    Patterns = ["*.bsp"],
                },
            ],
        });

        ViewModel?.AddBspFiles(files.Select(f => f.Path.LocalPath));
    }

    private async void BrowsePromptButton_Click(object? sender, Avalonia.Interactivity.RoutedEventArgs e)
    {
        var files = await StorageProvider.OpenFilePickerAsync(new FilePickerOpenOptions
        {
            Title = "Select prompt file",
            AllowMultiple = false,
            FileTypeFilter =
            [
                new FilePickerFileType("Markdown")
                {
                    Patterns = ["*.md"],
                },
                FilePickerFileTypes.TextPlain,
            ],
        });

        if (files.Count > 0 && ViewModel is not null)
        {
            ViewModel.PromptFilePath = files[0].Path.LocalPath;
        }
    }

    private void ClearButton_Click(object? sender, Avalonia.Interactivity.RoutedEventArgs e)
    {
        ViewModel?.Items.Clear();
    }

    private MainWindowViewModel? ViewModel => DataContext as MainWindowViewModel;
}
