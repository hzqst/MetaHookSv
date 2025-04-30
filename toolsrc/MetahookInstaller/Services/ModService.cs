using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using PeNet;

namespace MetahookInstaller.Services
{
    public class ModService
    {
        private readonly string _buildPath;
        private readonly string _gamePath;
        private readonly string _modName;
        private readonly string _sdl2Path;
        private readonly string _sdl3Path;

        public ModService(string buildPath, string gamePath, string modName, string sdl2Path, string sdl3Path)
        {
            _buildPath = buildPath ?? throw new ArgumentNullException(nameof(buildPath));
            _gamePath = gamePath ?? throw new ArgumentNullException(nameof(gamePath));
            _modName = modName ?? throw new ArgumentNullException(nameof(modName));
            _sdl2Path = sdl2Path;
            _sdl3Path = sdl3Path;
        }

        public void InstallMod(string gamePath, string modName, uint appId)
        {
            // 1. 复制svencoop文件夹
            var svencoopPath = Path.Combine(_buildPath, "svencoop");
            if (Directory.Exists(svencoopPath))
            {
                var targetModPath = Path.Combine(gamePath, modName);
                CopyDirectory(svencoopPath, targetModPath);
            }

            // 2. 复制mod特定文件夹
            var modFolders = Directory.GetDirectories(_buildPath)
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
                var platformPath = Path.Combine(_buildPath, "platform");
                if (Directory.Exists(platformPath))
                {
                    var targetPlatformPath = Path.Combine(gamePath, "platform");
                    CopyDirectory(platformPath, targetPlatformPath);
                }
            }

            // 4. 检查并复制MetaHook.exe或MetaHook_blob.exe
            var hwDllPath = Path.Combine(gamePath, "hw.dll");
            bool isLegitimatePE = IsLegitimatePE(hwDllPath);

            if (isLegitimatePE)
            {
                var metaHookPath = Path.Combine(_buildPath, "MetaHook.exe");
                if (File.Exists(metaHookPath))
                {
                    File.Copy(metaHookPath, Path.Combine(gamePath, "MetaHook.exe"), true);
                }
            }
            else
            {
                var metaHookBlobPath = Path.Combine(_buildPath, "MetaHook_blob.exe");
                if (File.Exists(metaHookBlobPath))
                {
                    File.Copy(metaHookBlobPath, Path.Combine(gamePath, "MetaHook.exe"), true);
                }
                else
                {
                    throw new Exception("找不到MetaHook_blob.exe，无法安装到非PE文件");
                }
            }

            // 5. 检查并复制SDL2.dll和SDL3.dll
            if (isLegitimatePE)
            {
                if (IsSDL2Imported(hwDllPath))
                {
                    if (File.Exists(_sdl2Path))
                    {
                        File.Copy(_sdl2Path, Path.Combine(gamePath, "SDL2.dll"), true);
                    }
                    if (File.Exists(_sdl3Path))
                    {
                        File.Copy(_sdl3Path, Path.Combine(gamePath, "SDL3.dll"), true);
                    }
                }
            }

            // 6. 检查{GameDirectory}\{ModDirectory}\metahook\configs\下面是否有 plugins.lst，如果没有，则将同目录下的 plugins_svencoop.lst（仅限Sven Co-op） 或 plugins_goldsrc.lst（非Sven Co-op游戏）重命名为 plugins.lst
            var configsPath = Path.Combine(gamePath, modName, "metahook", "configs");
            var pluginsLstPath = Path.Combine(configsPath, "plugins.lst");
            
            if (!File.Exists(pluginsLstPath) && Directory.Exists(configsPath))
            {
                string sourcePluginsFile;
                if (modName.Equals("svencoop", StringComparison.OrdinalIgnoreCase))
                {
                    sourcePluginsFile = Path.Combine(configsPath, "plugins_svencoop.lst");
                }
                else
                {
                    sourcePluginsFile = Path.Combine(configsPath, "plugins_goldsrc.lst");
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
        }

        public void UninstallMod(string gamePath, string modName)
        {
            var modPath = Path.Combine(gamePath, modName);
            if (Directory.Exists(modPath))
                Directory.Delete(modPath, true);

            var pluginsFile = Path.Combine(gamePath, "plugins.lst");
            if (File.Exists(pluginsFile))
            {
                var plugins = File.ReadAllLines(pluginsFile);
                var newPlugins = plugins.Where(p => p != modName).ToArray();
                File.WriteAllLines(pluginsFile, newPlugins);
            }
        }

        public void EnableMod(string gamePath, string modName)
        {
            var pluginsFile = Path.Combine(gamePath, "plugins.lst");
            var plugins = new List<string>();

            if (File.Exists(pluginsFile))
                plugins.AddRange(File.ReadAllLines(pluginsFile));

            if (!plugins.Contains(modName))
            {
                plugins.Add(modName);
                File.WriteAllLines(pluginsFile, plugins);
            }
        }

        public void DisableMod(string gamePath, string modName)
        {
            var pluginsFile = Path.Combine(gamePath, "plugins.lst");
            if (!File.Exists(pluginsFile))
                return;

            var plugins = File.ReadAllLines(pluginsFile);
            var newPlugins = plugins.Where(p => p != modName).ToArray();
            File.WriteAllLines(pluginsFile, newPlugins);
        }

        private void CopyDirectory(string sourceDir, string targetDir)
        {
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

        public List<string> LoadModList()
        {
            var pluginsFile = Path.Combine(_gamePath, _modName, "metahook", "configs", "plugins.lst");
            if (!File.Exists(pluginsFile))
                return new List<string>();

            return File.ReadAllLines(pluginsFile)
                .Where(line => !string.IsNullOrWhiteSpace(line) && !line.StartsWith("//"))
                .ToList();
        }

        public void SaveModList(List<string> mods)
        {
            var pluginsFile = Path.Combine(_gamePath, _modName, "metahook", "configs", "plugins.lst");
            File.WriteAllLines(pluginsFile, mods);
        }

        private bool IsLegitimatePE(string filePath)
        {
            try
            {
                var peFile = new PeFile(filePath);
                return peFile.ImageDosHeader != null && peFile.ImageNtHeaders != null;
            }
            catch
            {
                return false;
            }
        }

        private bool IsSDL2Imported(string filePath)
        {
            try
            {
                var peFile = new PeFile(filePath);
                if (peFile.ImageDosHeader == null || peFile.ImageNtHeaders == null)
                    return false;

                var imports = peFile.ImportedFunctions;
                return imports != null && imports.Any(i => i.DLL.ToLower() == "sdl2.dll");
            }
            catch
            {
                return false;
            }
        }
    }
} 