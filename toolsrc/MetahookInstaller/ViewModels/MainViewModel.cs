using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Windows;
using System.Windows.Threading;
using MetahookInstaller.Models;
using MetahookInstaller.Services;
using MetahookInstaller.Resources;

namespace MetahookInstaller.ViewModels
{
    public class MainViewModel : INotifyPropertyChanged
    {
        private readonly SteamService _steamService;
        private readonly ModService _modService;
        private GameInfo? _selectedGame;
        private string _gamePath = string.Empty;
        private string _modName = string.Empty;
        private uint _appId;

        // 添加一个事件来通知主窗口关闭
        public event EventHandler? RequestClose;

        public ObservableCollection<GameInfo> AvailableGames { get; } = new ObservableCollection<GameInfo>();

        public string ModName
        {
            get => _modName;
            set
            {
                if (_modName != value)
                {
                    _modName = value;
                    OnPropertyChanged(nameof(ModName));
                }
            }
        }

        public uint AppId
        {
            get => _appId;
            set
            {
                if (_appId != value)
                {
                    _appId = value;
                    OnPropertyChanged(nameof(AppId));
                }
            }
        }

        public bool IsCustomGame => SelectedGame?.AppId == 0;

        public GameInfo? SelectedGame
        {
            get => _selectedGame;
            set
            {
                if (_selectedGame != value)
                {
                    _selectedGame = value;
                    OnPropertyChanged(nameof(SelectedGame));
                    OnPropertyChanged(nameof(IsCustomGame));
                    UpdateGamePath();
                    if (value != null)
                    {
                        ModName = value.ModName;
                        AppId = value.AppId;
                    }
                }
            }
        }

        public string GamePath
        {
            get => _gamePath;
            set
            {
                if (_gamePath != value)
                {
                    _gamePath = value;
                    OnPropertyChanged(nameof(GamePath));
                }
            }
        }

        public MainViewModel()
        {
            _steamService = new SteamService();
            var buildPath = FindBuildPath();

            if (string.IsNullOrEmpty(buildPath))
            {
                MessageBox.Show(LocalizationService.GetString("BuildDirectoryNotFound"),
                              LocalizationService.GetString("Error"),
                              MessageBoxButton.OK,
                              MessageBoxImage.Error);
                // 使用Dispatcher来确保在MessageBox关闭后触发事件
                Application.Current.Dispatcher.BeginInvoke(new Action(() =>
                {
                    RequestClose?.Invoke(this, EventArgs.Empty);
                }), DispatcherPriority.Background);

                _modService = new ModService(string.Empty, string.Empty, string.Empty);
            }
            else
            {
                _modService = new ModService(buildPath, string.Empty, string.Empty);
            }
            LoadAvailableGames();
        }

        private string? FindBuildPath()
        {
            var baseDir = AppDomain.CurrentDomain.BaseDirectory;
#if DEBUG
            // Debug模式下搜索多级目录
            var possiblePaths = new[]
            {
                Path.Combine(baseDir, "Build"),
                Path.Combine(baseDir, "..", "Build"),
                Path.Combine(baseDir, "..", "..", "Build"),
                Path.Combine(baseDir, "..", "..", "..", "Build"),
                Path.Combine(baseDir, "..", "..", "..", "..", "Build"),
                Path.Combine(baseDir, "..", "..", "..", "..", "..", "Build")
            };
#else
            var possiblePaths = new[]
            {
                Path.Combine(baseDir, "Build"),
                Path.Combine(baseDir, "..", "Build"),
            };
#endif

            foreach (var path in possiblePaths)
            {
                if (Directory.Exists(path))
                {
                    var exePath1 = Path.Combine(path, "MetaHook.exe");
                    var exePath2 = Path.Combine(path, "MetaHook_blob.exe");
                    if (File.Exists(exePath1) || File.Exists(exePath2))
                    {
                        return path;
                    }
                }
            }

            return null;
        }

        private void LoadAvailableGames()
        {
            // 根据安装脚本更新游戏列表
            var games = new[]
            {
                new GameInfo("Sven Co-op", "svencoop", 225840, "svencoop"),
                new GameInfo("Half-Life", "valve", 70, "valve"),
                new GameInfo("Half-Life Updated", "halflife_updated", 70, "halflife_updated"),
                new GameInfo("Half-Life Opposing Force", "gearbox", 50, "gearbox"),
                new GameInfo("Half-Life Blue Shift", "bshift", 130, "bshift"),
                new GameInfo("Half-Life Echoes", "echoes", 70, "echoes"),
                new GameInfo("Half-Life MMod", "HL1MMod", 1761270, "HL1MMod"),
                new GameInfo("Counter-Strike", "cstrike", 10, "cstrike"),
                new GameInfo("Counter-Strike Condition Zero", "czero", 80, "czero"),
                new GameInfo("Counter-Strike Condition Zero - Deleted Scenes", "czeror", 100, "czeror"),
                new GameInfo("Day of Defeat", "dod", 30, "dod"),
                new GameInfo("aomdc", "aomdc", 70, "aomdc"),
                new GameInfo("Custom Game", "valve", 0, "valve"),
            };

            foreach (var game in games)
            {
                AvailableGames.Add(game);
            }

            if (AvailableGames.Any())
            {
                SelectedGame = AvailableGames.First();
            }
        }

        private void UpdateGamePath()
        {
            if (SelectedGame != null)
            {
                try
                {
                    if (SelectedGame.AppId == 0) // 自定义游戏
                    {
                        GamePath = string.Empty;
                        return;
                    }
                    var path = _steamService.GetGameInstallPath(SelectedGame.AppId);
                    // Normalize the path to ensure consistent directory separators
                    // This fixes the issue where mixed forward and backward slashes break shortcuts
                    GamePath = Path.GetFullPath(path);
                }
                catch (Exception ex)
                {
                    MessageBox.Show(string.Format(LocalizationService.GetString("ModInstallFailed"), ex.Message), 
                                  LocalizationService.GetString("Error"), 
                                  MessageBoxButton.OK, 
                                  MessageBoxImage.Error);
                    GamePath = string.Empty;
                }
            }
            else
            {
                GamePath = string.Empty;
            }
        }

        public void InstallMod()
        {
            if (SelectedGame == null)
            {
                MessageBox.Show(LocalizationService.GetString("PleaseSelectGame"),
                              LocalizationService.GetString("Error"),
                              MessageBoxButton.OK,
                              MessageBoxImage.Error);
                return;
            }
            if (string.IsNullOrEmpty(GamePath))
            {
                MessageBox.Show(LocalizationService.GetString("InvalidGameDirectory"),
                              LocalizationService.GetString("Error"),
                              MessageBoxButton.OK,
                              MessageBoxImage.Error);
                return;
            }

            var liblistGamPath = System.IO.Path.Combine(GamePath, ModName, "liblist.gam");
            if (!System.IO.File.Exists(liblistGamPath))
            {
                MessageBox.Show(string.Format(LocalizationService.GetString("InvalidModDirectory"), GamePath, ModName),
                              LocalizationService.GetString("Error"),
                              MessageBoxButton.OK,
                              MessageBoxImage.Error);
                return;
            }

            string GameName = SelectedGame.Name;

            if (AppId == 0)
            {
                string newGameName = _modService.GetGameNameFromLiblist(liblistGamPath);

                if (!string.IsNullOrEmpty(newGameName))
                {
                    GameName = newGameName;
                }
            }

            try
            {
                _modService.InstallMod(GamePath, ModName, AppId, GameName);
                MessageBox.Show(LocalizationService.GetString("ModInstallSuccess"),
                              LocalizationService.GetString("Success"),
                              MessageBoxButton.OK,
                              MessageBoxImage.Information);
            }
            catch (Exception ex)
            {
                MessageBox.Show(string.Format(LocalizationService.GetString("ModInstallFailed"), ex.Message),
                              LocalizationService.GetString("Error"),
                              MessageBoxButton.OK,
                              MessageBoxImage.Error);
            }
        }

        public void UninstallMod()
        {
            if (SelectedGame == null)
            {
                MessageBox.Show(LocalizationService.GetString("PleaseSelectGame"),
                              LocalizationService.GetString("Error"),
                              MessageBoxButton.OK,
                              MessageBoxImage.Error);
                return;
            }
            if (string.IsNullOrEmpty(GamePath))
            {
                MessageBox.Show(LocalizationService.GetString("InvalidGameDirectory"),
                              LocalizationService.GetString("Error"),
                              MessageBoxButton.OK,
                              MessageBoxImage.Error);
                return;
            }

            // Check if MetaHook is installed
            var metahookExePath = Path.Combine(GamePath, "MetaHook.exe");
            var metahookBlobExePath = Path.Combine(GamePath, "MetaHook_blob.exe");
            var svencoopExePath = Path.Combine(GamePath, "svencoop.exe");
            var metahookDirPath = Path.Combine(GamePath, ModName, "metahook");

            bool isInstalled = File.Exists(metahookExePath) ||
                             File.Exists(metahookBlobExePath) ||
                             File.Exists(svencoopExePath) ||
                             Directory.Exists(metahookDirPath);

            if (!isInstalled)
            {
                MessageBox.Show(LocalizationService.GetString("MetahookNotInstalledForUninstall"),
                              LocalizationService.GetString("Error"),
                              MessageBoxButton.OK,
                              MessageBoxImage.Error);
                return;
            }

            // Confirm uninstallation
            var result = MessageBox.Show(LocalizationService.GetString("ConfirmUninstall"),
                                       "MetaHook Installer",
                                       MessageBoxButton.YesNo,
                                       MessageBoxImage.Question);
            if (result != MessageBoxResult.Yes)
            {
                return;
            }

            string GameName = SelectedGame.Name;

            if (AppId == 0)
            {
                var liblistGamPath = Path.Combine(GamePath, ModName, "liblist.gam");
                if (File.Exists(liblistGamPath))
                {
                    string newGameName = _modService.GetGameNameFromLiblist(liblistGamPath);
                    if (!string.IsNullOrEmpty(newGameName))
                    {
                        GameName = newGameName;
                    }
                }
            }

            try
            {
                _modService.UninstallMod(GamePath, ModName, AppId, GameName);
                var successResult = MessageBox.Show(LocalizationService.GetString("UninstallSuccess"),
                                                  LocalizationService.GetString("Success"),
                                                  MessageBoxButton.OK,
                                                  MessageBoxImage.Information);
                
                // Open Steam help page after user clicks OK
                if (successResult == MessageBoxResult.OK)
                {
                    try
                    {
                        // Use Chinese URL if current culture is Chinese
                        var url = System.Globalization.CultureInfo.CurrentUICulture.Name == "zh-CN" 
                            ? "https://help.steampowered.com/zh-cn/faqs/view/0C48-FCBD-DA71-93EB"
                            : "https://help.steampowered.com/en/faqs/view/0C48-FCBD-DA71-93EB";
                            
                        System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo
                        {
                            FileName = url,
                            UseShellExecute = true
                        });
                    }
                    catch
                    {
                        // Ignore browser launch failure
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(string.Format(LocalizationService.GetString("UninstallFailed"), ex.Message),
                              LocalizationService.GetString("Error"),
                              MessageBoxButton.OK,
                              MessageBoxImage.Error);
            }
        }

        public event PropertyChangedEventHandler? PropertyChanged;

        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

} 