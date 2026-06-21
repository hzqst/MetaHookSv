using System.Globalization;
using System.Text;
using BSPLocalizationTools.GUI.Lang;
using ReactiveUI;

namespace BSPLocalizationTools.GUI.ViewModels;

public sealed partial class MainWindowViewModel
{
    private string _promptText = TranslationPromptBuilder.BuiltInPrompt;

    public string PromptText
    {
        get => _promptText;
        set => this.RaiseAndSetIfChanged(ref _promptText, value);
    }

    public void LoadBuiltInPrompt()
    {
        PromptText = TranslationPromptBuilder.BuiltInPrompt;
        AppendLog(Resources.LogLoadedBuiltInPrompt);
    }

    public void ReloadPromptFromFile()
    {
        LoadPromptFromConfiguredPath(logSuccess: true);
    }

    public bool SavePrompt()
    {
        var promptPath = Normalize(PromptFilePath);
        if (promptPath is null)
        {
            return false;
        }

        return SavePromptToFile(promptPath);
    }

    public bool SavePromptToFile(string promptPath)
    {
        try
        {
            var normalizedPath = Normalize(promptPath);
            if (normalizedPath is null)
            {
                AppendLog(string.Format(CultureInfo.CurrentCulture, Resources.LogFailedToSavePromptFile, Resources.PromptFileWatermark));
                return false;
            }

            var fullPath = Path.GetFullPath(normalizedPath);
            var directory = Path.GetDirectoryName(fullPath);
            if (!string.IsNullOrWhiteSpace(directory))
            {
                Directory.CreateDirectory(directory);
            }

            File.WriteAllText(fullPath, PromptText, new UTF8Encoding(encoderShouldEmitUTF8Identifier: false));
            PromptFilePath = fullPath;
            ToolConfigurationFile.Save(EnvPath, CreateConfiguration());
            AppendLog(string.Format(CultureInfo.CurrentCulture, Resources.LogSavedPromptFile, fullPath));
            return true;
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or NotSupportedException or ArgumentException)
        {
            AppendLog(string.Format(CultureInfo.CurrentCulture, Resources.LogFailedToSavePromptFile, ex.Message));
            return false;
        }
    }

    private void LoadPromptFromConfiguredPath(bool logSuccess)
    {
        var promptPath = Normalize(PromptFilePath);
        if (promptPath is not null && File.Exists(promptPath))
        {
            PromptText = File.ReadAllText(promptPath);
            if (logSuccess)
            {
                AppendLog(string.Format(CultureInfo.CurrentCulture, Resources.LogLoadedPromptFile, promptPath));
            }

            return;
        }

        PromptText = TranslationPromptBuilder.BuiltInPrompt;
        if (logSuccess)
        {
            AppendLog(Resources.LogLoadedBuiltInPrompt);
        }
    }
}
