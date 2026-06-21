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

    [Fact]
    public void TranslateToolbarExposesClearCompletedAndClearAllButtons()
    {
        var document = XDocument.Load(GetMainWindowXamlPath());
        XNamespace axaml = "https://github.com/avaloniaui";

        var buttons = document.Descendants(axaml + "Button").ToArray();

        Assert.Contains(buttons, e =>
            (string?)e.Attribute("Content") == "{Binding Strings.ClearCompletedTasks}" &&
            (string?)e.Attribute("Command") == "{Binding ClearCompletedItemsCommand}");
        Assert.Contains(buttons, e =>
            (string?)e.Attribute("Content") == "{Binding Strings.Clear}" &&
            (string?)e.Attribute("Command") == "{Binding ClearAllItemsCommand}");
    }

    [Fact]
    public void MapListContextMenuExposesRemoveAndRawGameTextActions()
    {
        var document = XDocument.Load(GetMainWindowXamlPath());
        XNamespace axaml = "https://github.com/avaloniaui";

        var menuItems = document.Descendants(axaml + "MenuItem").ToArray();

        Assert.Contains(menuItems, e =>
            (string?)e.Attribute("Header") == "{Binding #Root.DataContext.Strings.RemoveTask}" &&
            (string?)e.Attribute("Command") == "{Binding #Root.DataContext.RemoveItemCommand}" &&
            (string?)e.Attribute("CommandParameter") == "{Binding}");
        Assert.Contains(menuItems, e =>
            (string?)e.Attribute("Header") == "{Binding #Root.DataContext.Strings.ViewRawGameText}" &&
            (string?)e.Attribute("Click") == "ViewRawGameTextMenuItem_Click");
    }

    [Fact]
    public void MapListItemsExposeContextMenuAcrossFullRow()
    {
        var document = XDocument.Load(GetMainWindowXamlPath());
        XNamespace axaml = "https://github.com/avaloniaui";

        var itemGrid = document
            .Descendants(axaml + "DataTemplate")
            .Single(e => (string?)e.Attribute(XName.Get("DataType", "http://schemas.microsoft.com/winfx/2006/xaml")) == "vm:TranslationItemViewModel")
            .Element(axaml + "Grid");

        Assert.NotNull(itemGrid);
        Assert.Equal("Transparent", (string?)itemGrid.Attribute("Background"));
        Assert.Equal("Stretch", (string?)itemGrid.Attribute("HorizontalAlignment"));
    }

    [Fact]
    public void MapListStretchesGeneratedItemContainers()
    {
        var document = XDocument.Load(GetMainWindowXamlPath());
        XNamespace axaml = "https://github.com/avaloniaui";

        var listBox = document.Descendants(axaml + "ListBox").Single(e => (string?)e.Attribute("ItemsSource") == "{Binding Items}");
        var itemStyle = listBox
            .Element(axaml + "ListBox.Styles")
            ?.Elements(axaml + "Style")
            .SingleOrDefault(e => (string?)e.Attribute("Selector") == "ListBoxItem");
        var horizontalContentAlignment = itemStyle
            ?.Elements(axaml + "Setter")
            .SingleOrDefault(e => (string?)e.Attribute("Property") == "HorizontalContentAlignment");

        Assert.NotNull(horizontalContentAlignment);
        Assert.Equal("Stretch", (string?)horizontalContentAlignment.Attribute("Value"));
    }

    [Fact]
    public void MapListDeletesSelectedItemOnDeleteKey()
    {
        var document = XDocument.Load(GetMainWindowXamlPath());
        XNamespace axaml = "https://github.com/avaloniaui";

        var listBox = document.Descendants(axaml + "ListBox").Single(e => (string?)e.Attribute("ItemsSource") == "{Binding Items}");

        Assert.Equal("MapListBox", (string?)listBox.Attribute(XName.Get("Name", "http://schemas.microsoft.com/winfx/2006/xaml")));
        Assert.Equal("MapListBox_KeyDown", (string?)listBox.Attribute("KeyDown"));
    }

    [Fact]
    public void SettingsTabExposesAppendLanguageToCsvFileNameCheckBox()
    {
        var document = XDocument.Load(GetMainWindowXamlPath());
        XNamespace axaml = "https://github.com/avaloniaui";

        var checkBox = document.Descendants(axaml + "CheckBox").SingleOrDefault(e =>
            (string?)e.Attribute("IsChecked") == "{Binding AppendLanguageToCsvFileName}");

        Assert.NotNull(checkBox);
        Assert.Equal("{Binding Strings.AppendLanguageToCsvFileName}", (string?)checkBox.Attribute("Content"));
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
