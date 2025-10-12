# CLAUDE.md - MetahookInstaller

This file provides guidance to Claude Code when working with the MetahookInstaller project.

## Project Overview

MetahookInstaller is a WPF-based GUI installer tool for MetaHookSv. It automates the installation and configuration of MetaHook for various GoldSrc-based games, providing a user-friendly alternative to manual batch script installation.

### Key Features
- **Automatic Steam Game Detection** - Scans Steam library folders to locate installed games
- **Multi-Game Support** - Supports 12+ GoldSrc games including Sven Co-op, Half-Life, Counter-Strike, etc.
- **Intelligent Installation** - Detects blob vs non-blob engines and installs appropriate MetaHook versions
- **Plugin Management** - Visual plugin list editor with drag-and-drop reordering
- **Localization** - Multi-language support (English, 简体中文)
- **Desktop Shortcut Creation** - Automatically creates game launch shortcuts

## Technology Stack

### Framework & Language
- **.NET 8.0** (`net8.0-windows`) - Target framework
- **WPF (Windows Presentation Foundation)** - UI framework
- **C# 12** with nullable reference types enabled
- **MVVM (Model-View-ViewModel)** architectural pattern

### Key Dependencies
```xml
<PackageReference Include="PeNet" Version="5.1.0" />                    <!-- PE file parsing -->
<PackageReference Include="securifybv.ShellLink" Version="0.1.0" />    <!-- Shortcut creation -->
<PackageReference Include="System.Windows.Forms" Version="4.0.0" />    <!-- Folder browser dialog -->
<PackageReference Include="System.Drawing.Common" Version="9.0.4" />   <!-- Icon handling -->
```

### Project File
- **MetahookInstaller.csproj** - .NET SDK-style project with WPF support

## Project Structure

```
toolsrc/MetahookInstaller/
├── App.xaml / App.xaml.cs           # Application entry point
├── AssemblyInfo.cs                  # Assembly metadata
├── Models/
│   └── GameInfo.cs                  # Game configuration data model
├── ViewModels/
│   └── MainViewModel.cs             # Main window business logic (MVVM)
├── Views/
│   ├── MainWindow.xaml/.cs          # Main installation UI
│   └── PluginEditorDialog.xaml/.cs  # Plugin list editor dialog
├── Services/
│   ├── ModService.cs                # Installation logic & file operations
│   ├── SteamService.cs              # Steam integration & game path detection
│   └── LocalizationService.cs       # Localization helper
├── Converters/
│   ├── InverseBooleanConverter.cs   # XAML boolean inverter
│   └── ModStateConverter.cs         # Mod state display converter
├── Resources/
│   ├── Strings.resx                 # Default (English) strings
│   ├── Strings.zh-CN.resx           # Simplified Chinese strings
│   └── Strings.Designer.cs          # Auto-generated resource accessor
└── Properties/
    └── Resources.resx               # Embedded resources

Build Output: bin/Debug/net8.0-windows/ or bin/Release/net8.0-windows/
```

## Core Components

### 1. MainViewModel.cs
The main ViewModel implementing `INotifyPropertyChanged`:
- **Game Selection** - Manages `AvailableGames` collection and `SelectedGame` property
- **Path Detection** - Auto-detects game paths via `SteamService`
- **Custom Game Support** - Allows manual path/AppId entry for non-Steam games
- **Build Path Discovery** - Searches for `Build/` directory (multi-level search in Debug mode)
- **Installation Trigger** - Validates inputs and invokes `ModService.InstallMod()`

Key Methods:
- `FindBuildPath()` - Locates MetaHook build directory
- `LoadAvailableGames()` - Populates game list (matches `install-*.bat` scripts)
- `UpdateGamePath()` - Updates path when game selection changes
- `InstallMod()` - Validates and executes installation

### 2. ModService.cs
Core installation logic service:

**Installation Process (`InstallMod`):**
1. Copy `Build/svencoop/` to `{GamePath}/{ModName}/`
2. Copy mod-specific folders (e.g., `Build/cstrike/` → `{GamePath}/cstrike/`)
3. For Sven Co-op: Copy `Build/platform/` → `{GamePath}/platform/`
4. **Engine Detection:**
   - Check `hw.dll` with `IsLegitimatePE()` (PeNet parser)
   - Non-blob: Copy `MetaHook.exe` (or `svencoop.exe` for Sven Co-op)
   - Blob: Copy `MetaHook_blob.exe`
5. **SDL2 Detection:**
   - Check `hw.dll` imports with `IsSDL2Imported()`
   - Copy `SDL2.dll` and `SDL3.dll` if needed
6. **Plugin List Setup:**
   - If `plugins.lst` doesn't exist:
     - Sven Co-op: Copy `plugins_svencoop.lst` → `plugins.lst`
     - Other games: Copy `plugins_goldsrc.lst` → `plugins.lst`
7. Delete template files: `plugins_svencoop.lst`, `plugins_goldsrc.lst`
8. Create desktop shortcut: `MetaHook for {GameName}.lnk`

Key Methods:
- `GetGameNameFromLiblist()` - Parses `liblist.gam` for game name
- `CopyDirectory()` - Recursive directory copy with overwrite
- `IsLegitimatePE()` - Validates PE file format (blob vs non-blob detection)
- `IsSDL2Imported()` - Checks DLL import table for SDL2 dependency

### 3. SteamService.cs
Steam integration service:

**Features:**
- Registry-based Steam path detection (`HKCU\Software\Valve\Steam\SteamPath`)
- Multi-library support (parses `steamapps/libraryfolders.vdf`)
- Manifest parsing (`appmanifest_{appId}.acf`) to find game install directories

Key Methods:
- `GetGameInstallPath(uint appId)` - Returns full path to game installation
- `GetSteamPath()` - Reads Steam path from registry
- `GetLibraryFolders()` - Parses VDF to find all Steam library locations
- `GetInstallDirFromManifest()` - Extracts `installdir` field from ACF manifest

### 4. GameInfo Model
Represents a supported game:
```csharp
public class GameInfo
{
    public string Name { get; set; }       // Display name (e.g., "Sven Co-op")
    public string ShortName { get; set; }  // Internal name (e.g., "svencoop")
    public uint AppId { get; set; }        // Steam AppId (0 = custom game)
    public string ModName { get; set; }    // Mod directory name
}
```

### 5. Plugin Editor (PluginEditorDialog)
UI for editing `plugins.lst`:
- Left panel: Enabled plugins with up/down reordering
- Right panel: Available plugins (read-only reference)
- Checkbox state toggles plugin enable/disable (comment/uncomment lines)

## Development Workflow

### Building
```bash
# Build from Visual Studio
Open MetaHook.sln → Build MetahookInstaller project

# Or via dotnet CLI
cd toolsrc/MetahookInstaller
dotnet build -c Release
```

**Output Location:**
- Debug: `bin/Debug/net8.0-windows/MetahookInstaller.exe`
- Release: `bin/Release/net8.0-windows/MetahookInstaller.exe`

### Running & Debugging
1. Set `MetahookInstaller` as startup project in Visual Studio
2. Ensure `Build/` directory exists with MetaHook binaries
3. Press F5 to debug
4. For Release testing: Copy built EXE to repository root (same level as `Build/`)

**Important:** The installer searches for `Build/` directory relative to its location:
- Debug mode: Searches up to 5 parent directories
- Release mode: Searches current and parent directory only

### Adding New Game Support

1. **Update `LoadAvailableGames()` in MainViewModel.cs:**
```csharp
new GameInfo("Game Name", "shortname", steamAppId, "modname")
```

2. **Create corresponding build files:**
```
Build/
├── {modname}/           # Mod-specific files
└── {modname}_*/         # Additional mod folders (optional)
```

3. **Create `plugins_*.lst` templates:**
```
Build/{modname}/metahook/configs/
├── plugins_goldsrc.lst  # For most GoldSrc games
└── plugins_svencoop.lst # For Sven Co-op only
```

4. **Test installation workflow:**
- Launch installer
- Select new game
- Verify file copy operations
- Check shortcut creation
- Validate plugin list initialization

### Adding Localization

1. **Open `Resources/Strings.resx`** in Visual Studio resource editor
2. **Add new key-value pair** for English string
3. **Open `Resources/Strings.zh-CN.resx`** (or create new culture file)
4. **Add corresponding translated string**
5. **Rebuild project** to regenerate `Strings.Designer.cs`
6. **Use in XAML:**
```xml
<TextBlock Text="{x:Static res:Strings.YourNewKey}"/>
```
7. **Use in C#:**
```csharp
LocalizationService.GetString("YourNewKey")
```

## Important Notes

### Engine Type Detection
The installer uses PE file analysis to determine engine type:
- **Blob engines** (3248~4554): Encrypted `hw.dll`, requires `MetaHook_blob.exe`
- **Non-blob engines** (6153+): Standard PE format, uses `MetaHook.exe`

Detection logic in `ModService.IsLegitimatePE()`:
```csharp
var peFile = new PeFile(hwDllPath);
return peFile.ImageDosHeader != null && peFile.ImageNtHeaders != null;
```

### SDL2 Detection
Modern GoldSrc engines (8684+, Half-Life 25th Anniversary) use SDL2:
```csharp
var imports = peFile.ImportedFunctions;
return imports.Any(i => i.DLL.ToLower() == "sdl2.dll");
```

### Custom Game Mode
When `AppId == 0`:
- Manual path input required
- Uses `liblist.gam` to determine game name
- Allows modding of non-Steam GoldSrc games

### File Overwrite Behavior
All file copy operations use `overwrite: true` to ensure clean installations and updates.

### Shortcut Parameters
Generated shortcuts include:
- **Target:** `{GamePath}/MetaHook.exe` (or `svencoop.exe`)
- **Arguments:** `-insecure -game {modname}`
- **Working Directory:** `{GamePath}`
- **Icon:** Extracted from MetaHook executable

## MVVM Pattern in This Project

### Model
- `GameInfo` - Pure data class

### View
- `MainWindow.xaml` - Installation UI
- `PluginEditorDialog.xaml` - Plugin manager UI

### ViewModel
- `MainViewModel` - Main window logic with `INotifyPropertyChanged`
- No separate ViewModel for PluginEditor (uses code-behind for simplicity)

### Services
- `ModService` - Business logic (installation)
- `SteamService` - External integration (Steam API)
- `LocalizationService` - Utility service

### Data Binding Examples
```xml
<!-- Two-way binding with property change notification -->
<ComboBox SelectedItem="{Binding SelectedGame}"/>

<!-- One-way binding with converter -->
<TextBox IsReadOnly="{Binding IsCustomGame, Converter={StaticResource InverseBooleanConverter}}"/>

<!-- Static resource binding -->
<TextBlock Text="{x:Static res:Strings.GamePath}"/>
```

## Testing Considerations

### Manual Test Checklist
- [ ] Steam path detection works
- [ ] Multi-library Steam setup supported
- [ ] Blob engine detected correctly (test with CS 1.6 buildnum < 6153)
- [ ] Non-blob engine detected correctly (test with Sven Co-op)
- [ ] SDL2 games copy SDL2.dll/SDL3.dll (test with HL25)
- [ ] Shortcut created with correct parameters
- [ ] Plugin list initialized from correct template
- [ ] Custom game mode works without Steam
- [ ] Localization switches based on system culture
- [ ] Plugin editor saves changes correctly

### Known Limitations
- Windows-only (WPF, Windows Forms dialogs, Registry access)
- Requires Steam for automatic game detection (manual mode available)
- No rollback/uninstall functionality
- No version checking or update detection

## Related Files in Main Repository

### Installation Scripts (Reference Implementation)
The installer replicates functionality from:
```
scripts/
├── install-helper-AIO.bat          # Main installation logic (batch equivalent)
├── install-helper-CopyBuild.bat    # File copy operations
├── install-helper-CopySDL2.bat     # SDL2 detection & copy
├── install-helper-CreateShortcut.bat # Shortcut creation
└── install-to-*.bat                # Per-game installation scripts
```

### Build Output Structure
```
Build/
├── MetaHook.exe                    # Non-blob loader
├── MetaHook_blob.exe               # Blob loader
├── SDL2.dll / SDL3.dll             # SDL libraries
├── svencoop/                       # Base mod files
│   └── metahook/
│       ├── configs/
│       │   ├── plugins_svencoop.lst
│       │   └── plugins_goldsrc.lst
│       └── plugins/                # Plugin DLLs
└── {modname}/                      # Mod-specific overrides
```

## Security & Best Practices

This is a **defensive tool** for legitimate game modification:
- ✅ File copying and directory management
- ✅ PE file analysis for engine detection
- ✅ Shortcut creation for user convenience
- ✅ Read-only Steam registry/file access
- ❌ No network operations
- ❌ No credential handling
- ❌ No process injection or memory manipulation

### Code Safety
- Uses `Path.GetFullPath()` to normalize paths
- Validates file existence before operations
- Uses try-catch with user-friendly error messages
- No hardcoded paths (relative path resolution)

## Troubleshooting

### "Build directory not found" error
- Ensure `Build/` folder exists at same level as installer EXE
- Check for `MetaHook.exe` or `MetaHook_blob.exe` inside Build/

### "Game not found" error
- Verify game is installed via Steam
- Check Steam library folders configuration
- Use "Custom Game" mode for non-Steam installations

### Shortcut not working
- Verify shortcut target path is absolute
- Check working directory is set correctly
- Ensure `-game {modname}` parameter matches actual directory

### Plugin list not created
- Check `plugins_goldsrc.lst` or `plugins_svencoop.lst` exists in Build/
- Verify `metahook/configs/` directory structure

## Future Enhancement Ideas

- **Auto-update detection** - Check for newer MetaHook releases
- **Backup/restore** - Save original files before overwrite
- **Plugin repository** - Download/install community plugins
- **Multi-mod support** - Install to multiple mods simultaneously
- **Configuration presets** - Save/load plugin configurations
- **Diagnostic tool** - Verify installation integrity
- **Uninstaller** - Remove MetaHook files cleanly
