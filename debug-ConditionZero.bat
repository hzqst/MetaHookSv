echo off

set LauncherExe=metahook.exe
set LauncherMod=czero

for /f "delims=" %%a in ('%~dp0SteamAppsLocation/SteamAppsLocation 80 InstallDir') do set GameDir=%%a

if "%GameDir%"=="" goto fail

echo -----------------------------------------------------

echo Writing debug configuration...
copy global_template.props global.props /y
call powershell -Command "(gc global.props) -replace '<MetaHookLaunchName>.*</MetaHookLaunchName>', '<MetaHookLaunchName>%LauncherExe%</MetaHookLaunchName>' | Out-File global.props"
call powershell -Command "(gc global.props) -replace '<MetaHookLaunchCommnand>.*</MetaHookLaunchCommnand>', '<MetaHookLaunchCommnand>-game %LauncherMod%</MetaHookLaunchCommnand>' | Out-File global.props"
call powershell -Command "(gc global.props) -replace '<MetaHookGameDirectory>.*</MetaHookGameDirectory>', '<MetaHookGameDirectory>%GameDir%\</MetaHookGameDirectory>' | Out-File global.props"
call powershell -Command "(gc global.props) -replace '<MetaHookModName>.*</MetaHookModName>', '<MetaHookModName>%LauncherMod%</MetaHookModName>' | Out-File global.props"

echo -----------------------------------------------------

echo done
pause
exit

:fail

echo Failed to locate GameInstallDir of Counter-Strike : Condition Zero, please make sure Steam is running and you have Counter-Strike : Condition Zero installed correctly.
pause
exit