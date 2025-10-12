using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Shapes;
using System.Windows.Threading;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls.Primitives;
using System.Windows.Media.Animation;
using System.Windows.Media.Imaging;
using System.Windows.Media.Media3D;
using System.Windows.Media.TextFormatting;
using System.Windows.Navigation;
using System.IO;
using System.Collections.ObjectModel;
using MetahookInstaller.ViewModels;
using System.Text.RegularExpressions;
using MetahookInstaller.Services;
using MetahookInstaller.Resources;

namespace MetahookInstaller.Views
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private readonly MainViewModel _viewModel;

        public MainWindow()
        {
            LocalizationService.Initialize();
            InitializeComponent();
            _viewModel = new MainViewModel();
            _viewModel.RequestClose += (s, e) => Close();
            DataContext = _viewModel;
        }

        private void InstallButton_Click(object sender, RoutedEventArgs e)
        {
            _viewModel.InstallMod();
        }

        private void UninstallButton_Click(object sender, RoutedEventArgs e)
        {
            _viewModel.UninstallMod();
        }

        private void NumberValidationTextBox(object sender, TextCompositionEventArgs e)
        {
            Regex regex = new Regex("[^0-9]+");
            e.Handled = regex.IsMatch(e.Text);
        }

        private void EditPluginListButton_Click(object sender, RoutedEventArgs e)
        {
            if (_viewModel.SelectedGame != null)
            {
                var dialog = new PluginEditorDialog(_viewModel.GamePath, _viewModel.SelectedGame.ModName);
                dialog.Owner = this;
                dialog.ShowDialog();
            }
        }

        private void BrowseGamePath_Click(object sender, RoutedEventArgs e)
        {
            using (var dialog = new System.Windows.Forms.FolderBrowserDialog())
            {
                dialog.Description = LocalizationService.GetString("SelectGameDirectory");
                
                if (dialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
                {
                    var hwDllPath = System.IO.Path.Combine(dialog.SelectedPath, "hw.dll");
                    if (!System.IO.File.Exists(hwDllPath))
                    {
                        MessageBox.Show(LocalizationService.GetString("InvalidGameDirectory"), 
                                      LocalizationService.GetString("Error"), 
                                      MessageBoxButton.OK, 
                                      MessageBoxImage.Error);
                        return;
                    }
                    
                    _viewModel.GamePath = dialog.SelectedPath;
                }
            }
        }
    }
} 