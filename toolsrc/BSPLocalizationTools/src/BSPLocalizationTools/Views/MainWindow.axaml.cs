using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Layout;
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

    private async void SaveConfigurationButton_Click(object? sender, Avalonia.Interactivity.RoutedEventArgs e)
    {
        if (ViewModel is null)
        {
            return;
        }

        ViewModel.SaveConfiguration();
        await ShowMessageDialogAsync(
            ViewModel.Strings.SaveConfigurationSucceededTitle,
            ViewModel.Strings.SaveConfigurationSucceededMessage,
            ViewModel.Strings.Ok);
    }

    private async void ViewRawGameTextMenuItem_Click(object? sender, Avalonia.Interactivity.RoutedEventArgs e)
    {
        if (sender is not MenuItem { DataContext: TranslationItemViewModel item } || ViewModel is null)
        {
            return;
        }

        var dialog = new Window
        {
            Title = string.Format(
                System.Globalization.CultureInfo.CurrentCulture,
                ViewModel.Strings.RawGameTextTitle,
                item.FileName),
            Width = 720,
            Height = 520,
            MinWidth = 520,
            MinHeight = 360,
            Content = new Border
            {
                Padding = new Avalonia.Thickness(12),
                Child = new TextBox
                {
                    Text = ViewModel.GetRawGameText(item),
                    IsReadOnly = true,
                    AcceptsReturn = true,
                    TextWrapping = Avalonia.Media.TextWrapping.Wrap,
                    VerticalContentAlignment = VerticalAlignment.Top,
                },
            },
        };

        await dialog.ShowDialog(this);
    }

    private void MapListBox_KeyDown(object? sender, KeyEventArgs e)
    {
        if (e.Key != Key.Delete ||
            sender is not ListBox { SelectedItem: TranslationItemViewModel item } ||
            ViewModel is null ||
            !ViewModel.RemoveItemCommand.CanExecute(item))
        {
            return;
        }

        ViewModel.RemoveItemCommand.Execute(item);
        e.Handled = true;
    }

    private async Task ShowMessageDialogAsync(string title, string message, string okText)
    {
        var okButton = new Button
        {
            Content = okText,
            HorizontalAlignment = HorizontalAlignment.Right,
            MinWidth = 84,
        };
        Window? dialog = null;
        okButton.Click += (_, _) => dialog?.Close();

        dialog = new Window
        {
            Title = title,
            Width = 380,
            Height = 160,
            MinWidth = 320,
            MinHeight = 140,
            WindowStartupLocation = WindowStartupLocation.CenterOwner,
            Content = new Border
            {
                Padding = new Avalonia.Thickness(16),
                Child = new StackPanel
                {
                    Spacing = 16,
                    Children =
                    {
                        new TextBlock
                        {
                            Text = message,
                            TextWrapping = Avalonia.Media.TextWrapping.Wrap,
                        },
                        okButton,
                    },
                },
            },
        };

        await dialog.ShowDialog(this);
    }

    private MainWindowViewModel? ViewModel => DataContext as MainWindowViewModel;
}
