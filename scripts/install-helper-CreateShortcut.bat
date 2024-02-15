@echo off

cd /d "%SolutionDir%"

if exist "%SolutionDir%Build\MetaHook.exe" ( 

    powershell $shell = New-Object -ComObject WScript.Shell;$shortcut = $shell.CreateShortcut(\"MetaHook for %ShortGameName%.lnk\");$shortcut.TargetPath = \"%GameDir%\%LauncherExe%\";$shortcut.WorkingDirectory = \"%GameDir%\";$shortcut.Arguments = \"-insecure -steam -game %LauncherMod%\";$shortcut.Save();

) else (
   
    powershell $shell = New-Object -ComObject WScript.Shell;$shortcut = $shell.CreateShortcut(\"MetaHook for %ShortGameName%.lnk\");$shortcut.TargetPath = \"%GameDir%\%LauncherExe%\";$shortcut.WorkingDirectory = \"%GameDir%\";$shortcut.Arguments = \"-insecure -game %LauncherMod%\";$shortcut.Save();

)
echo -----------------------------------------------------
echo Please launch game from shortcut "MetaHook for %ShortGameName%"

start explorer "%SolutionDir%"

pause

exit