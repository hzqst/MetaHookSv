using System;
using System.Runtime.InteropServices;
using System.Text;
using System.IO;
using Microsoft.Win32;

namespace MetahookInstaller.Services
{
    public class SteamService
    {
        [DllImport("steam_api64.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool SteamAPI_Init();

        [DllImport("steam_api64.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void SteamAPI_Shutdown();

        [DllImport("steam_api64.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr SteamClient();

        [DllImport("steam_api64.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool SteamAPI_IsSteamRunning();

        [UnmanagedFunctionPointer(CallingConvention.ThisCall)]
        private delegate IntPtr GetISteamAppsDelegate(IntPtr self, IntPtr hSteamUser, IntPtr hSteamPipe, string pchVersion);

        [UnmanagedFunctionPointer(CallingConvention.ThisCall)]
        private delegate int GetAppInstallDirDelegate(IntPtr self, uint appId, StringBuilder pchFolder, int cubFolder);

        private string? _steamPath;

        public string GetGameInstallPath(uint appId)
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

            throw new Exception($"Could not found game with AppId {appId}");
        }

        private string? GetSteamPath()
        {
            using var key = Registry.CurrentUser.OpenSubKey(@"Software\Valve\Steam");
            if (key == null)
                return null;

            return key.GetValue("SteamPath") as string;
        }

        private string[] GetLibraryFolders()
        {
            if (_steamPath == null)
                return Array.Empty<string>();

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

            return libraryFolders.ToArray();
        }

        private string? GetInstallDirFromManifest(string manifest)
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
    }
} 