# Suggested Commands for MetaHookSv Development

## Build Requirements
- Visual Studio 2022 (vc143 toolset)
- CMake
- Git for Windows

## Build Configurations
- `Debug`
- `Release`
- `Release_AVX2` (Renderer and BulletPhysics AVX2 builds)
- `Release_blob` (legacy blob engines)

## Build Commands

### Build MetaHook Loader
```batch
scripts\build-MetaHook.bat
```
Builds the main MetaHook.exe (and MetaHook_blob.exe for legacy engines).

### Build All Plugins
```batch
scripts\build-Plugins.bat
```
Builds all plugin DLLs and utility libraries in the correct dependency order.

### Build Specific Plugin (using MSBuild directly)
```batch
MSBuild.exe MetaHook.sln "/target:Plugins\Renderer" /p:Configuration="Release" /p:Platform="Win32"
```

## Debug Commands

(Other game debug scripts live under `scripts/` as `debug-*.bat`.)

### Debug with Sven Co-op
```batch
scripts\debug-SvenCoop.bat
```

### Debug with Counter-Strike
```batch
scripts\debug-CounterStrike.bat
```

### Debug with Half-Life
```batch
scripts\debug-HalfLife.bat
```

## Installation Commands

### Install to Sven Co-op
```batch
scripts\install-to-SvenCoop.bat
```

### Install to Counter-Strike
```batch
scripts\install-to-CounterStrike.bat
```

## Git Commands (Windows)
```batch
git status
git add .
git commit -m "message"
git push
git pull
git log
git diff
```

## Windows System Commands
```batch
dir                    # List directory contents
cd <path>              # Change directory
type <file>            # Display file contents
copy <src> <dest>      # Copy files
del <file>             # Delete file
mkdir <dir>            # Create directory
rmdir <dir>            # Remove directory
```

## Visual Studio Commands
- Set target plugin as startup project
- Press F5 to start debugging
- Ctrl+Shift+B to build solution
