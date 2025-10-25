using Avalonia.Controls.Notifications;
using Avalonia.Media.Imaging;
using MetahookInstallerAvalonia.Handler;
using MetahookInstallerAvalonia.Lang;
using Microsoft.Win32;
using ReactiveUI;
using ShellLink;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reactive.Linq;
using System.Windows.Input;
using Ursa.Controls;
using Notification = Ursa.Controls.Notification;
using Path = System.IO.Path;
using WindowNotificationManager = Ursa.Controls.WindowNotificationManager;

namespace MetahookInstallerAvalonia.ViewModels;

public class MainViewModel : ViewModelBase
{
    public WindowNotificationManager? NotificationManager { get; set; }
    #region Page 1
    private static string? FindBuildPath()
    {
#if DEBUG
        // Debug模式下搜索多级目录
        var possiblePaths = new[]
        {
                Path.GetFullPath("Build"),
                Path.GetFullPath(Path.Combine("..", "Build")),
                Path.GetFullPath(Path.Combine("..", "..", "Build")),
                Path.GetFullPath(Path.Combine("..", "..", "..", "Build")),
                Path.GetFullPath(Path.Combine("..", "..", "..", "..", "Build")),
                Path.GetFullPath(Path.Combine("..", "..", "..", "..", "..", "Build"))
            };
#else
            var possiblePaths = new[]
            {
                Path.GetFullPath("Build"),
                Path.GetFullPath(Path.Combine("..", "Build")),
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

    private static void CopyDirectory(string sourceDir, string targetDir)
    {
        if (!Directory.Exists(targetDir))
            Directory.CreateDirectory(targetDir);
        foreach (var file in Directory.GetFiles(sourceDir))
        {
            var targetFile = Path.Combine(targetDir, Path.GetFileName(file));
            File.Copy(file, targetFile, true);
        }
        foreach (var dir in Directory.GetDirectories(sourceDir))
        {
            var targetSubDir = Path.Combine(targetDir, Path.GetFileName(dir));
            CopyDirectory(dir, targetSubDir);
        }
    }

    public static bool IsLegitimatePE(string dllPath)
    {
        if (!File.Exists(dllPath))
            return false;
        try
        {
            using var stream = new FileStream(dllPath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
            using var br = new BinaryReader(stream);
            if (stream.Length < 2 || stream.Length < 0x3C + 4)
                return false;
            if (br.ReadUInt16() != 0x5A4D)
                return false;
            stream.Position = 0x3C;
            int lfanew = br.ReadInt32();
            if (lfanew < 0 || lfanew + 4 > stream.Length)
                return false;
            stream.Position = lfanew;
            if (br.ReadUInt32() != 0x00004550)
                return false;
            return true;
        }
        catch
        {
            return false;
        }
    }
    private static long RvaToFileOffset(BinaryReader stream, int lfanew, uint rva)
    {
        try
        {
            stream.BaseStream.Position = lfanew + 4 + 20;
            ushort sizeOfOptionalHeader = stream.ReadUInt16();
            long sectionTableStart = lfanew + 4 + 20 + 2 + sizeOfOptionalHeader;
            stream.BaseStream.Position = lfanew + 4 + 2;
            ushort numberOfSections = stream.ReadUInt16();
            for (int i = 0; i < numberOfSections; i++)
            {
                long sectionPosition = sectionTableStart + i * 40;
                if (sectionPosition + 40 > stream.BaseStream.Length)
                    break;
                stream.BaseStream.Position = sectionPosition + 12;
                uint virtualAddress = stream.ReadUInt32();
                stream.BaseStream.Position = sectionPosition + 16;
                uint sizeOfRawData = stream.ReadUInt32();
                stream.BaseStream.Position = sectionPosition + 20;
                uint pointerToRawData = stream.ReadUInt32();
                if (rva >= virtualAddress && rva < virtualAddress + sizeOfRawData)
                {
                    return rva - virtualAddress + pointerToRawData;
                }
            }
            return -1;
        }
        catch
        {
            return -1;
        }
    }
    public static bool HasImportedModule(string dllPath, string targetModule)
    {
        if (!File.Exists(dllPath))
            return false;

        targetModule = targetModule.ToLowerInvariant();
        try
        {
            using var stream = new FileStream(dllPath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
            using var reader = new BinaryReader(stream);
            if (stream.Length < 0x3C + 4)
                return false;
            stream.Position = 0x3C;
            int lfanew = reader.ReadInt32();
            if (lfanew + 4 > stream.Length)
                return false;
            stream.Position = lfanew;
            if (reader.ReadUInt32() != 0x00004550)
                return false;
            stream.Position = lfanew + 4; // 跳过 PE 签名
            ushort machine = reader.ReadUInt16();
            reader.ReadUInt16(); // 跳过 NumberOfSections
            reader.ReadUInt32(); // 跳过 TimeDateStamp
            reader.ReadUInt32(); // 跳过 PointerToSymbolTable
            reader.ReadUInt32(); // 跳过 NumberOfSymbols
            ushort sizeOfOptionalHeader = reader.ReadUInt16();
            reader.ReadUInt16(); // 跳过 Characteristics
            long optionalHeaderStart = stream.Position;
            bool is64Bit = machine == 0x8664; // 0x8664 表示 x64
            int dataDirectoryOffset = is64Bit ? 224 : 208;
            if (optionalHeaderStart + dataDirectoryOffset + 8 > stream.Length)
                return false; // 数据目录表位置无效

            // 5. 读取导入表在数据目录中的地址（相对虚拟地址 RVA）和大小
            stream.Position = optionalHeaderStart + dataDirectoryOffset;
            uint importTableRva = reader.ReadUInt32(); // 导入表的 RVA
            uint importTableSize = reader.ReadUInt32(); // 导入表大小（若为0则无导入表）
            if (importTableSize == 0)
                return false;

            // 6. 将 RVA 转换为文件偏移量（需要通过节表计算）
            long importTableFileOffset = RvaToFileOffset(reader, lfanew, importTableRva);
            if (importTableFileOffset == -1)
                return false;

            // 7. 遍历导入表中的每个 IMAGE_IMPORT_DESCRIPTOR
            stream.Position = importTableFileOffset;
            while (true)
            {
                // 读取一个导入描述符（简化版，仅关注名称 RVA）
                uint originalFirstThunk = reader.ReadUInt32();
                reader.ReadUInt32(); // 跳过 TimeDateStamp
                reader.ReadUInt32(); // 跳过 ForwarderChain
                uint nameRva = reader.ReadUInt32(); // 模块名称的 RVA
                reader.ReadUInt32(); // 跳过 FirstThunk

                // 若所有字段为0，则表示导入表结束
                if (originalFirstThunk == 0 && nameRva == 0)
                    break;

                // 8. 解析模块名称
                if (nameRva == 0)
                    continue;
                long nameFileOffset = RvaToFileOffset(reader, lfanew, nameRva);
                if (nameFileOffset == -1)
                    continue;
                stream.Position = nameFileOffset;
                string moduleName = reader.ReadString().ToLowerInvariant();
                if (moduleName == targetModule)
                    return true;
            }
            return false;
        }
        catch
        {
            return false;
        }
    }

    private void InstallMod(string basePath)
    {
        if (Selected == null || Selected.GamePath == null || string.IsNullOrEmpty(Selected.GamePath) || !File.Exists(Path.Combine(Selected.InstallPath, "liblist.gam")))
        {
            MessageBox.ShowAsync(Resources.InvalidInstallPath, Resources.CriticalError, MessageBoxIcon.Error, MessageBoxButton.OK);
            return;
        }
        // 1. 复制svencoop文件夹
        var svencoopPath = Path.Combine(basePath, "svencoop");
        if (Directory.Exists(svencoopPath))
        {

            CopyDirectory(svencoopPath, Selected.InstallPath);
        }

        string modName = Selected.Directory;
        string gamePath = Selected.GamePath;
        // 2. 复制mod特定文件夹
        var modFolders = Directory.GetDirectories(basePath)
            .Where(d => Path.GetFileName(d).Equals(modName, StringComparison.OrdinalIgnoreCase) ||
                       Path.GetFileName(d).StartsWith($"{modName}_", StringComparison.OrdinalIgnoreCase));
        foreach (var folder in modFolders)
        {
            var targetPath = Path.Combine(gamePath, Path.GetFileName(folder));
            CopyDirectory(folder, targetPath);
        }

        // 3. 如果是Sven Co-op，复制platform文件夹
        if (modName.Equals("svencoop", StringComparison.OrdinalIgnoreCase))
        {
            var platformPath = Path.Combine(basePath, "platform");
            if (Directory.Exists(platformPath))
            {
                var targetPlatformPath = Path.Combine(gamePath, "platform");
                CopyDirectory(platformPath, targetPlatformPath);
            }
        }

        // 4. 检查并复制MetaHook.exe或MetaHook_blob.exe
        var hwDllPath = Path.Combine(gamePath, "hw.dll");
        var sourceMetaHookPath = Path.Combine(basePath, "MetaHook.exe");
        var targetMetaHookPath = Path.Combine(gamePath, "MetaHook.exe");
        bool isNonBlobEngine = IsLegitimatePE(hwDllPath);
        if (isNonBlobEngine)
        {
            if (modName.Equals("svencoop", StringComparison.OrdinalIgnoreCase))
            {
                targetMetaHookPath = Path.Combine(gamePath, "svencoop.exe");
            }
            else
            {
                targetMetaHookPath = Path.Combine(gamePath, "MetaHook.exe");
            }
        }
        else
        {
            sourceMetaHookPath = Path.Combine(basePath, "MetaHook_blob.exe");
            targetMetaHookPath = Path.Combine(gamePath, "MetaHook_blob.exe");
        }
        if (File.Exists(sourceMetaHookPath))
        {
            File.Copy(sourceMetaHookPath, targetMetaHookPath, true);
        }
        else
        {
            MessageBox.ShowAsync(Resources.FileNotFound, Resources.CriticalError, MessageBoxIcon.Error, MessageBoxButton.OK);
            throw new Exception("Fatal Error: Could not found " + sourceMetaHookPath);
        }
        // 5. 检查并复制SDL2.dll和SDL3.dll
        if (isNonBlobEngine)
        {
            if (HasImportedModule(hwDllPath, "sdl2.dll"))
            {
                var sdl2Path = Path.Combine(basePath, "sdl2.dll");
                var sdl3Path = Path.Combine(basePath, "sdl3.dll");

                if (File.Exists(sdl2Path))
                {
                    File.Copy(sdl2Path, Path.Combine(gamePath, "sdl2.dll"), true);
                }
                if (File.Exists(sdl3Path))
                {
                    File.Copy(sdl3Path, Path.Combine(gamePath, "sdl3.dll"), true);
                }
            }
        }
        // 6. 检查{GameDirectory}\{ModDirectory}\metahook\configs\下面是否有 plugins.lst，如果没有，则将同目录下的 plugins_svencoop.lst（仅限Sven Co-op） 或 plugins_goldsrc.lst（非Sven Co-op游戏）重命名为 plugins.lst
        var configsPath = Path.Combine(gamePath, modName, "metahook", "configs");
        var pluginsLstPath = Path.Combine(configsPath, "plugins.lst");

        if (!File.Exists(pluginsLstPath) && Directory.Exists(configsPath))
        {
            var sourcePluginsFile = Path.Combine(configsPath, "plugins_goldsrc.lst");

            if (modName.Equals("svencoop", StringComparison.OrdinalIgnoreCase))
            {
                sourcePluginsFile = Path.Combine(configsPath, "plugins_svencoop.lst");
            }

            if (File.Exists(sourcePluginsFile))
            {
                File.Copy(sourcePluginsFile, pluginsLstPath, true);
            }
        }

        // 7. 删除{GameDirectory}\{ModDirectory}\metahook\configs\plugins_svencoop.lst 和 {GameDirectory}\{ModDirectory}\metahook\configs\plugins_goldsrc.lst
        var pluginsSvencoopPath = Path.Combine(configsPath, "plugins_svencoop.lst");
        var pluginsGoldsrcPath = Path.Combine(configsPath, "plugins_goldsrc.lst");

        if (File.Exists(pluginsSvencoopPath))
        {
            File.Delete(pluginsSvencoopPath);
        }

        if (File.Exists(pluginsGoldsrcPath))
        {
            File.Delete(pluginsGoldsrcPath);
        }

        // 8. 为 targetMetaHookPath 创建快捷方式至当前MetahookInstaller.exe所在目录
        var installerPath = Path.GetFullPath(".");
        var shortcutPath = Path.Combine(installerPath, $"MetaHook for {Selected.Name}.lnk");
        Shortcut lnk = Shortcut.CreateShortcut(
          targetMetaHookPath,
            $"-insecure -game {modName}",
            gamePath,
            targetMetaHookPath,
            0);
        lnk.WriteToFile(shortcutPath);
        NotificationManager?.Show(new Notification(
                            Resources.Success,
                            Resources.InstallDone,
                            NotificationType.Success,
                            new TimeSpan(0, 0, 5)
                            ));
        _pluginListInitialized = false;
        this.RaisePropertyChanged(nameof(EditorUsable));
    }
    private readonly ICommand _install;
    public ICommand InstallCommand => _install;

    private void UninstallMod()
    {
        if (Selected == null || Selected.GamePath == null)
        {
            MessageBox.ShowAsync(Resources.InvalidInstallPath, Resources.CriticalError, MessageBoxIcon.Error, MessageBoxButton.OK);
            return;
        }
        string modName = Selected.Directory;
        string gamePath = Selected.GamePath;
        string[] list = [
            $"{modName}/metahook/",
            $"{modName}/renderer/",
            $"{modName}/scmodeldownloader/",
            $"{modName}/vgui2ext/",
            $"{modName}/captionmod/",
            $"{modName}/bulletphysics/",
            $"{modName}/sprites/radio_external.txt",
            $"{modName}/sprites/voiceicon_external.txt",
            "MetaHook.exe",
            "MetaHook_blob.exe"
            ];
        foreach (var item in list)
        {
            if (item.EndsWith('/'))
            {
                var dirPath = Path.Combine(gamePath, item.TrimEnd('/'));
                if (Directory.Exists(dirPath))
                {
                    try
                    {
                        Directory.Delete(dirPath, true);
                    }
                    catch (Exception ex)
                    {
                        NotificationManager?.Show(new Notification(
                            Resources.Warning,
                            string.Format(Resources.DeleteFailed, dirPath, ex.Message),
                            NotificationType.Warning
                            ));
                    }
                }
            }
            else
            {
                var filePath = Path.Combine(gamePath, item);
                if (File.Exists(filePath))
                {
                    try
                    {
                        File.Delete(filePath);
                    }
                    catch (Exception ex)
                    {
                        NotificationManager?.Show(new Notification(
                            Resources.Warning,
                            string.Format(Resources.DeleteFailed, filePath, ex.Message),
                            NotificationType.Warning
                            ));
                    }
                }
            }
        }
        // Delete desktop shortcut
        var installerPath = Path.GetFullPath(".");
        if (installerPath != null)
        {
            var shortcutPath = Path.Combine(installerPath, $"MetaHook for {Selected.Name}.lnk");
            if (File.Exists(shortcutPath))
            {
                try
                {
                    File.Delete(shortcutPath);
                }
                catch
                {
                    // Ignore shortcut deletion failure
                }
            }
        }

        NotificationManager?.Show(new Notification(
                            Resources.Success,
                            Resources.UninstallDone,
                            NotificationType.Success,
                            new TimeSpan(0, 0, 5)
                            ));
        _pluginListInitialized = false;
        this.RaisePropertyChanged(nameof(EditorUsable));
    }
    private readonly ICommand _uninstall;
    public ICommand UninstallCommand => _uninstall;

    public class ModInfo(string name, string directory, uint appid, string path)
    {
        private string _name = name;
        private string _directory = directory;
        private uint _appid = appid;
        private string? _path = path;
        private readonly string _installpath = Path.Combine(path, directory);
        private readonly Bitmap? _icon = (!string.IsNullOrEmpty(Path.Combine(path, directory, "game.ico")) &&
            File.Exists(Path.Combine(path, directory, "game.ico"))) ? new Bitmap(Path.Combine(path, directory, "game.ico")) : null;
        private bool _readonly = true;

        public string Name { get => _name; set => _name = value; }
        public string Directory { get => _directory; set => _directory = value; }
        public uint AppID { get => _appid; set => _appid = value; }
        public string? GamePath { get => _path; set => _path = value; }
        public string InstallPath { get => _installpath; }
        public Bitmap? GameIcon => _icon;
        public bool ReadOnly { get => _readonly; set => _readonly = value; }
    }
    private readonly ObservableCollection<ModInfo> _modInfos = [];
    private ModInfo? _selected = null;
    public ObservableCollection<ModInfo> ModInfos => _modInfos;
    public ModInfo? Selected
    {
        get => _selected;
        set => this.RaiseAndSetIfChanged(ref _selected, value);
    }
    private string? _steamPath = null;
    private string[] GetLibraryFolders()
    {
        if (_steamPath == null)
            return [];
        var libraryFolders = new List<string> { _steamPath };
        var configPath = Path.Combine(_steamPath, "steamapps", "libraryfolders.vdf");
        if (File.Exists(configPath))
        {
            var config = File.ReadAllText(configPath);
            var lines = config.Split('\n');
            foreach (var line in lines)
            {
                if (line.Contains("\"path\""))
                {
                    var path = line.Split('"')[3].Replace("\\\\", "\\");
                    libraryFolders.Add(path);
                }
            }
        }

        return [.. libraryFolders];
    }
    private static string? GetInstallDirFromManifest(string manifest)
    {
        var lines = manifest.Split('\n');
        foreach (var line in lines)
        {
            if (line.Contains("\"installdir\""))
            {
                return line.Split('"')[3];
            }
        }
        return null;
    }
    private static string? GetSteamPath()
    {
        using var key = Registry.CurrentUser.OpenSubKey(@"Software\Valve\Steam");
        if (key == null)
            return null;

        return key.GetValue("SteamPath") as string;
    }
    public string? GetGameInstallPath(uint appId)
    {
        if (_steamPath == null)
        {
            _steamPath = GetSteamPath();
            if (_steamPath == null)
                throw new Exception("Could not found Steam path");
        }

        var libraryFolders = GetLibraryFolders();
        foreach (var library in libraryFolders)
        {
            var gamePath = Path.Combine(library, "steamapps", "common");
            if (!Directory.Exists(gamePath))
                continue;

            var manifestPath = Path.Combine(library, "steamapps", $"appmanifest_{appId}.acf");
            if (!File.Exists(manifestPath))
                continue;

            var manifest = File.ReadAllText(manifestPath);
            var installDir = GetInstallDirFromManifest(manifest);
            if (string.IsNullOrEmpty(installDir))
                continue;

            var fullPath = Path.Combine(gamePath, installDir);
            if (Directory.Exists(fullPath))
                return fullPath;
        }

        return null;
    }
    #endregion


    #region Page 2
    public class PluginInfoComparer : IEqualityComparer<PluginInfo>
    {
        public bool Equals(PluginInfo? x, PluginInfo? y)
        {
            if (x == null && y == null)
                return true;
            if (x == null || y == null)
                return false;
            return string.Equals(x.Name, y.Name, StringComparison.OrdinalIgnoreCase);

        }
        public int GetHashCode(PluginInfo obj)
        {
            return obj?.Name?.GetHashCode() ?? 0;
        }
    }
    private readonly ObservableCollection<PluginInfo> _plugins = [];
    public ObservableCollection<PluginInfo> Plugins => _plugins;
    private readonly ObservableCollection<PluginInfo> _avaliable = [];
    public ObservableCollection<PluginInfo> Avaliable => _avaliable;
    private PluginInfo? _selectedPlugin = null;
    private PluginInfo? _selectedAvaliable = null;
    public PluginInfo? SelectedPlugin
    {
        get => _selectedPlugin;
        set => this.RaiseAndSetIfChanged(ref _selectedPlugin, value);
    }
    public PluginInfo? SelectedAvaliable
    {
        get => _selectedAvaliable;
        set => this.RaiseAndSetIfChanged(ref _selectedAvaliable, value);
    }
    public bool EditorUsable => IsEditorUsable();

    private int _selectedTabIndex;
    private bool _pluginListInitialized = false;
    public int SelectedTabIndex
    {
        get => _selectedTabIndex;
        set => this.RaiseAndSetIfChanged(ref _selectedTabIndex, value);
    }

    private readonly ICommand _toAvaliable;
    private readonly ICommand _toPlugins;
    public ICommand ToAvaliableCommand => _toAvaliable;
    public ICommand ToPluginsCommand => _toPlugins;

    private bool IsEditorUsable()
    {
        if (Selected == null || Selected.GamePath == null)
            return false;
        string _gamePath = Selected.GamePath;
        string _modName = Selected.Directory;
        var pluginsLstPath = Path.Combine(_gamePath, _modName, "metahook", "configs", "plugins.lst");
        var pluginsDir = Path.Combine(_gamePath, _modName, "metahook", "plugins");
        if (!File.Exists(pluginsLstPath) || !Directory.Exists(pluginsDir))
            return false;
        return true;
    }

    public bool InitPluginList()
    {
        _plugins.Clear();
        _avaliable.Clear();
        if (Selected == null || Selected.GamePath == null)
        {
            MessageBox.ShowAsync(Resources.SelectFirst, Resources.Warning, MessageBoxIcon.Warning, MessageBoxButton.OK);
            return false;
        }
        string _gamePath = Selected.GamePath;
        string _modName = Selected.Directory;
        var pluginsLstPath = Path.Combine(_gamePath, _modName, "metahook", "configs", "plugins.lst");
        var pluginsDir = Path.Combine(_gamePath, _modName, "metahook", "plugins");

        if (!File.Exists(pluginsLstPath) || !Directory.Exists(pluginsDir))
        {
            MessageBox.ShowAsync(Resources.SelectFirst, Resources.Warning, MessageBoxIcon.Warning, MessageBoxButton.OK);
            return false;
        }

        var lines = File.ReadAllLines(pluginsLstPath);
        List<PluginInfo> ps = [];
        foreach (var line in lines)
        {
            if (line != null && !string.IsNullOrWhiteSpace(line))
            {
                var plugininfo = new PluginInfo(line.TrimStart(';').Trim(), !line.StartsWith(';'));
                ps.Add(plugininfo);
            }
        }
        ps = [.. ps.Distinct(new PluginInfoComparer())];

        List<PluginInfo> aps = [];
        if (Directory.Exists(pluginsDir))
        {
            foreach (var dll in Directory.GetFiles(pluginsDir, "*.dll"))
            {
                var pluginName = Path.GetFileName(dll);
                if (pluginName.EndsWith("_AVX2.dll"))
                {
                    pluginName = pluginName.Replace("_AVX2.dll", ".dll");
                }
                var plugininfo = new PluginInfo(pluginName.Trim(), false);
                aps.Add(plugininfo);
            }
        }
        aps = [.. aps.Distinct(new PluginInfoComparer()).Where(a => !ps.Any(p => a.Name.Equals(p.Name, StringComparison.OrdinalIgnoreCase)))];

        foreach (var p in ps)
        {
            _plugins.Add(p);
        }
        foreach (var p in aps)
        {
            _avaliable.Add(p);
        }
        NotificationManager?.Show(new Notification(
                            Resources.Success,
                            Resources.ResetDone,
                            NotificationType.Success,
                            new TimeSpan(0, 0, 5)
                            ));
        return true;
    }
    private void SavePluginList()
    {
        if (Selected == null || Selected.GamePath == null)
        {
            MessageBox.ShowAsync(Resources.SelectFirst, Resources.Warning, MessageBoxIcon.Warning, MessageBoxButton.OK);
            return;
        }
        string _gamePath = Selected.GamePath;
        string _modName = Selected.Directory;
        var pluginsLstPath = Path.Combine(_gamePath, _modName, "metahook", "configs", "plugins.lst");

        if (!File.Exists(pluginsLstPath))
        {
            MessageBox.ShowAsync(Resources.SelectFirst, Resources.Warning, MessageBoxIcon.Warning, MessageBoxButton.OK);
            return;
        }

        using StreamWriter sw = new(pluginsLstPath);
        foreach (var p in _plugins)
        {
            string text = $"{(p.Enabled ? "" : ';')}{p.Name}";
            sw.WriteLine(text);
        }
        NotificationManager?.Show(new Notification(
                            Resources.Success,
                            Resources.SaveDone,
                            NotificationType.Success,
                            new TimeSpan(0, 0, 5)
                            ));
    }
    private readonly ICommand _save;
    public ICommand SaveCommand => _save;

    private readonly ICommand _reset;
    public ICommand ResetCommand => _reset;
    #endregion

    private readonly ICommand _changeLanguage;
    public ICommand ChangeLanguageCommand => _changeLanguage;

    public MainViewModel()
    {
        #region Setup Games
        (string, string, uint)[] list = [
            new ("Sven Co-op", "svencoop", 225840),
            new ("Half-Life", "valve", 70),
            new ("Half-Life Updated", "halflife_updated", 70),
            new ("Half-Life Opposing Force", "gearbox", 50),
            new ("Half-Life Blue Shift", "bshift", 130),
            new ("Half-Life Echoes", "echoes", 70),
            new ("Half-Life MMod", "HL1MMod", 1761270),
            new ("Counter-Strike", "cstrike", 10),
            new ("Counter-Strike Condition Zero", "czero", 80),
            new ("Counter-Strike Condition Zero - Deleted Scenes", "czeror", 100),
            new ("Day of Defeat", "dod", 30),
            new ("Afraid of Monsters: Director's Cut", "aomdc", 70)];

        foreach (var m in list)
        {
            if (GetGameInstallPath(m.Item3) is string path)
            {
                var info = new ModInfo(m.Item1, m.Item2, m.Item3, path);
                if (Directory.Exists(info.InstallPath))
                {
                    _modInfos.Add(info);
                }
            }
        }
        var custom = new ModInfo(Resources.CustomGame, "", 0, "")
        {
            ReadOnly = false
        };
        _modInfos.Add(custom);
        _selected = _modInfos.FirstOrDefault();
        this.WhenAnyValue(x => x.Selected)
            .Subscribe(_ =>
            {
                this.RaisePropertyChanged(nameof(EditorUsable));
                _pluginListInitialized = false;
            });
        this.WhenAnyValue(x => x.SelectedTabIndex)
            .Where(idx => idx == 1) // 第二个 Tab 的索引为 1
            .Subscribe(_ =>
            {
                if (!_pluginListInitialized)
                {
                    InitPluginList();
                    _pluginListInitialized = true;
                }
            });
        #endregion

        #region Setup Commands
        _install = new Command(
            _ =>
            {
                var buildPath = FindBuildPath();
                if (string.IsNullOrEmpty(buildPath))
                {
                    MessageBox.ShowAsync(Resources.BuildDirectoryNotFound, Resources.CriticalError, MessageBoxIcon.Error, MessageBoxButton.OK);
                    return;
                }
                InstallMod(buildPath);
            },
            _ => true
        );
        _uninstall = new Command(
            _ =>
            {
                UninstallMod();
            },
            _ => true
        );
        _toAvaliable = new Command(
            _ =>
            {
                if (SelectedPlugin is PluginInfo plugin)
                {
                    Plugins.Remove(plugin);
                    Avaliable.Add(plugin);
                }
            },
            _ => true
         );
        _toPlugins = new Command(
            _ =>
            {
                if (SelectedAvaliable is PluginInfo plugin)
                {
                    Avaliable.Remove(plugin);
                    Plugins.Add(plugin);
                }
            },
            _ => true
        );
        _save = new Command(
            _ =>
            {
                SavePluginList();
            },
            _ => true
            );
        _reset = new Command(
            _ =>
            {
                InitPluginList();
            },
            _ => true
            );
        _changeLanguage = new Command(
           obj =>
           {
               if (obj is string lang)
               {
                   var settingPath = Path.Combine(".", "lang");
                   using StreamWriter sw = new(settingPath);
                   sw.Write(lang);
                   sw.Flush();
                   string? currentExePath = Process.GetCurrentProcess().MainModule?.FileName;
                   if (currentExePath == null || string.IsNullOrEmpty(currentExePath))
                   {
                       currentExePath = Environment.GetCommandLineArgs()[0];
                   }
                   string[] args = Environment.GetCommandLineArgs().Skip(1).ToArray();
                   string arguments = string.Join(" ", args);
                   var startInfo = new ProcessStartInfo
                   {
                       FileName = currentExePath,
                       Arguments = arguments,
                       UseShellExecute = true
                   };
                   Process.Start(startInfo);
                   Environment.Exit(0);
               }
           },
           _ => true
       );
        #endregion
    }
}
