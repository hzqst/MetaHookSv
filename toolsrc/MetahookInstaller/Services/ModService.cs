using System.IO;
using PeNet;
using DK.WshRuntime;
using File = System.IO.File;

namespace MetahookInstaller.Services
{
    public class ModService
    {
        private readonly string _buildPath;

        public ModService(string buildPath, string gamePath, string modName)
        {
            _buildPath = buildPath ?? throw new ArgumentNullException(nameof(buildPath));
        }

        public void InstallMod(string gamePath, string modName, uint appId, string modFullName)
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
            var sourceMetaHookPath = Path.Combine(_buildPath, "MetaHook.exe");
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
                sourceMetaHookPath = Path.Combine(_buildPath, "MetaHook_blob.exe");
                targetMetaHookPath = Path.Combine(gamePath, "MetaHook_blob.exe");
            }

            if (File.Exists(sourceMetaHookPath))
            {
                File.Copy(sourceMetaHookPath, targetMetaHookPath, true);
            }
            else
            {
                throw new Exception("Fatal Error: Could not found " + sourceMetaHookPath);
            }

            // 5. 检查并复制SDL2.dll和SDL3.dll
            if (isNonBlobEngine)
            {
                if (IsSDL2Imported(hwDllPath))
                {
                    var sdl2Path = Path.Combine(_buildPath, "SDL2.dll");
                    var sdl3Path = Path.Combine(_buildPath, "SDL3.dll");

                    if (File.Exists(sdl2Path))
                    {
                        File.Copy(sdl2Path, Path.Combine(gamePath, "SDL2.dll"), true);
                    }
                    if (File.Exists(sdl3Path))
                    {
                        File.Copy(sdl3Path, Path.Combine(gamePath, "SDL3.dll"), true);
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
            var installerPath = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
            if (installerPath == null)
            {
                throw new InvalidOperationException("Fatal Error: Could not found installer path");
            }
            var shortcutPath = Path.Combine(installerPath, $"MetaHook for {modFullName}.lnk");
            
            WshInterop.CreateShortcut(
                shortcutPath,
                $"MetaHook for {modFullName}",
                targetMetaHookPath,
                $"-insecure -game {modName}",
                targetMetaHookPath
            );
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