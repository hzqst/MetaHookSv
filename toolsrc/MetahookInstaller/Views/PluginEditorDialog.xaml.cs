using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Windows;
using System.Windows.Threading;
using MetahookInstaller.Services;
using MetahookInstaller.Resources;

namespace MetahookInstaller.Views
{
    public partial class PluginEditorDialog : Window
    {
        private readonly string _gamePath;
        private readonly string _modName;
        private readonly ObservableCollection<PluginItem> _enabledPlugins;
        private readonly ObservableCollection<PluginItem> _availablePlugins;

        public PluginEditorDialog(string gamePath, string modName)
        {
            InitializeComponent();
            _gamePath = gamePath;
            _modName = modName;
            _enabledPlugins = new ObservableCollection<PluginItem>();
            _availablePlugins = new ObservableCollection<PluginItem>();

            EnabledPluginsListView.ItemsSource = _enabledPlugins;
            AvailablePluginsListView.ItemsSource = _availablePlugins;

            LoadPlugins();
        }

        private void LoadPlugins()
        {
            var pluginsLstPath = Path.Combine(_gamePath, _modName, "metahook", "configs", "plugins.lst");
            var pluginsDir = Path.Combine(_gamePath, _modName, "metahook", "plugins");

            if (!File.Exists(pluginsLstPath) || !Directory.Exists(pluginsDir))
            {
                MessageBox.Show(LocalizationService.GetString("MetahookNotInstalled"), 
                              LocalizationService.GetString("Error"), 
                              MessageBoxButton.OK, 
                              MessageBoxImage.Error);
                Dispatcher.BeginInvoke(new Action(() => 
                {
                    DialogResult = false;
                    Close();
                }));
                return;
            }

            // 读取已启用的插件
            var enabledPlugins = new HashSet<string>();
            foreach (var line in File.ReadAllLines(pluginsLstPath))
            {
                var pluginName = line.Trim();
                if (!string.IsNullOrEmpty(pluginName) && !pluginName.StartsWith(";"))
                {
                    enabledPlugins.Add(pluginName);
                }
            }

            // 获取所有可用插件
            var availablePlugins = new HashSet<string>();
            if (Directory.Exists(pluginsDir))
            {
                foreach (var dll in Directory.GetFiles(pluginsDir, "*.dll"))
                {
                    var pluginName = Path.GetFileName(dll);
                    if (pluginName.EndsWith("_AVX2.dll"))
                    {
                        // 如果是AVX2版本，使用普通版本
                        pluginName = pluginName.Replace("_AVX2.dll", ".dll");
                    }
                    availablePlugins.Add(pluginName);
                }
            }

            // 填充列表
            foreach (var plugin in availablePlugins.OrderBy(p => p))
            {
                var isEnabled = enabledPlugins.Contains(plugin);
                _enabledPlugins.Add(new PluginItem(plugin, isEnabled));
                _availablePlugins.Add(new PluginItem(plugin, false));
            }
        }

        private void SaveButton_Click(object sender, RoutedEventArgs e)
        {
            var pluginsLstPath = Path.Combine(_gamePath, _modName, "metahook", "configs", "plugins.lst");
            var lines = new List<string>();

            foreach (var plugin in _enabledPlugins)
            {
                if (plugin.IsEnabled)
                {
                    lines.Add(plugin.Name);
                }
                else
                {
                    lines.Add($";{plugin.Name}");
                }
            }

            File.WriteAllLines(pluginsLstPath, lines);
            DialogResult = true;
            Close();
        }

        private void CancelButton_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
            Close();
        }

        private void MoveUpButton_Click(object sender, RoutedEventArgs e)
        {
            var selectedIndex = EnabledPluginsListView.SelectedIndex;
            if (selectedIndex > 0)
            {
                var item = _enabledPlugins[selectedIndex];
                _enabledPlugins.RemoveAt(selectedIndex);
                _enabledPlugins.Insert(selectedIndex - 1, item);
                EnabledPluginsListView.SelectedIndex = selectedIndex - 1;
            }
        }

        private void MoveDownButton_Click(object sender, RoutedEventArgs e)
        {
            var selectedIndex = EnabledPluginsListView.SelectedIndex;
            if (selectedIndex >= 0 && selectedIndex < _enabledPlugins.Count - 1)
            {
                var item = _enabledPlugins[selectedIndex];
                _enabledPlugins.RemoveAt(selectedIndex);
                _enabledPlugins.Insert(selectedIndex + 1, item);
                EnabledPluginsListView.SelectedIndex = selectedIndex + 1;
            }
        }
    }

    public class PluginItem
    {
        public string Name { get; set; }
        public bool IsEnabled { get; set; }

        public PluginItem(string name, bool isEnabled)
        {
            Name = name;
            IsEnabled = isEnabled;
        }
    }
} 