using System.Xml.Linq;

namespace BSPLocalizationTools.Tests;

public sealed class MainWindowLayoutTests
{
    [Fact]
    public void SettingsTabContentReservesSpaceForVerticalScrollBar()
    {
        var document = XDocument.Load(GetMainWindowXamlPath());
        XNamespace axaml = "https://github.com/avaloniaui";
        var settingsTab = document
            .Descendants(axaml + "TabItem")
            .Single(e => (string?)e.Attribute("Header") == "{Binding Strings.SettingsTab}");
        var scrollContent = settingsTab.Element(axaml + "ScrollViewer")?.Element(axaml + "Border");

        Assert.NotNull(scrollContent);
        Assert.Equal("0,0,20,0", (string?)scrollContent.Attribute("Padding"));
    }

    private static string GetMainWindowXamlPath()
    {
        var directory = new DirectoryInfo(AppContext.BaseDirectory);
        while (directory is not null && directory.Name != "BSPLocalizationTools")
        {
            directory = directory.Parent;
        }

        Assert.NotNull(directory);
        return Path.Combine(directory.FullName, "src", "BSPLocalizationTools", "Views", "MainWindow.axaml");
    }
}
