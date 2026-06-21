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
            Title = ViewModel?.Strings.SelectBspFilesTitle ?? "Select BSP files",
            AllowMultiple = true,
            FileTypeFilter =
            [
                new FilePickerFileType(ViewModel?.Strings.GoldSrcBspFilter ?? "GoldSrc BSP")
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
            Title = ViewModel?.Strings.SelectPromptFileTitle ?? "Select prompt file",
            AllowMultiple = false,
            FileTypeFilter =
            [
                new FilePickerFileType(ViewModel?.Strings.MarkdownFilter ?? "Markdown")
                {
                    Patterns = ["*.md"],
                },
                FilePickerFileTypes.TextPlain,
            ],
        });

        if (files.Count > 0 && ViewModel is not null)
        {
            ViewModel.PromptFilePath = files[0].Path.LocalPath;
            ViewModel.ReloadPromptFromFile();
        }
    }

    private async void SavePromptButton_Click(object? sender, Avalonia.Interactivity.RoutedEventArgs e)
    {
        if (ViewModel is null || ViewModel.SavePrompt())
        {
            return;
        }

        var file = await StorageProvider.SaveFilePickerAsync(new FilePickerSaveOptions
        {
            Title = ViewModel.Strings.SavePromptFileTitle,
            SuggestedFileName = "prompt.md",
            DefaultExtension = "md",
            FileTypeChoices =
            [
                new FilePickerFileType(ViewModel.Strings.MarkdownFilter)
                {
                    Patterns = ["*.md"],
                },
                FilePickerFileTypes.TextPlain,
            ],
        });

        if (file is not null)
        {
            ViewModel.SavePromptToFile(file.Path.LocalPath);
        }
    }

    private void ClearButton_Click(object? sender, Avalonia.Interactivity.RoutedEventArgs e)
    {
        ViewModel?.Items.Clear();
    }

    private MainWindowViewModel? ViewModel => DataContext as MainWindowViewModel;
}
