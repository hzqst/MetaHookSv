echo off

set LauncherExe=metahook.exe
set LauncherMod=valve

for /f "delims=" %%a in ('%~dp0SteamAppsLocation/SteamAppsLocation 70 InstallDir') do set GameDir=%%a

if "%GameDir%"=="" goto fail

copy "%~dp0Build\%LauncherExe%" "%GameDir%\%LauncherExe%" /y
copy "%~dp0Build\SDL2.dll" "%GameDir%\" /y
copy "%~dp0Build\FreeImage.dll" "%GameDir%\" /y
xcopy "%~dp0Build\svencoop\" "%GameDir%\%LauncherMod%\" /y /e

echo -----------------------------------------------------

echo Writing debug configuration...

if not exist "MetaHook.vcxproj.user" copy "vcxproj.user.template" "MetaHook.vcxproj.user"

call powershell -Command "(gc MetaHook.vcxproj.user) -replace '<LocalDebuggerCommand>.*</LocalDebuggerCommand>', '<LocalDebuggerCommand>%LauncherExe%</LocalDebuggerCommand>' | Out-File MetaHook.vcxproj.user"
call powershell -Command "(gc MetaHook.vcxproj.user) -replace '<LocalDebuggerCommandArguments>.*</LocalDebuggerCommandArguments>', '<LocalDebuggerCommandArguments>-game %LauncherMod%</LocalDebuggerCommandArguments>' | Out-File MetaHook.vcxproj.user"
call powershell -Command "(gc MetaHook.vcxproj.user) -replace '<LocalDebuggerWorkingDirectory>.*</LocalDebuggerWorkingDirectory>', '<LocalDebuggerWorkingDirectory>%GameDir%\</LocalDebuggerWorkingDirectory>' | Out-File MetaHook.vcxproj.user"

if not exist "Plugins\BulletPhysics\BulletPhysics.vcxproj.user" copy "vcxproj.user.template" "Plugins\BulletPhysics\BulletPhysics.vcxproj.user"

call powershell -Command "(gc Plugins\BulletPhysics\BulletPhysics.vcxproj.user) -replace '<LocalDebuggerCommand>.*</LocalDebuggerCommand>', '<LocalDebuggerCommand>%LauncherExe%</LocalDebuggerCommand>' | Out-File Plugins\BulletPhysics\BulletPhysics.vcxproj.user"
call powershell -Command "(gc Plugins\BulletPhysics\BulletPhysics.vcxproj.user) -replace '<LocalDebuggerCommandArguments>.*</LocalDebuggerCommandArguments>', '<LocalDebuggerCommandArguments>-game %LauncherMod%</LocalDebuggerCommandArguments>' | Out-File Plugins\BulletPhysics\BulletPhysics.vcxproj.user"
call powershell -Command "(gc Plugins\BulletPhysics\BulletPhysics.vcxproj.user) -replace '<LocalDebuggerWorkingDirectory>.*</LocalDebuggerWorkingDirectory>', '<LocalDebuggerWorkingDirectory>%GameDir%\</LocalDebuggerWorkingDirectory>' | Out-File Plugins\BulletPhysics\BulletPhysics.vcxproj.user"

if not exist "Plugins\CaptionMod\CaptionMod.vcxproj.user" copy "vcxproj.user.template" "Plugins\CaptionMod\CaptionMod.vcxproj.user"

call powershell -Command "(gc Plugins\CaptionMod\CaptionMod.vcxproj.user) -replace '<LocalDebuggerCommand>.*</LocalDebuggerCommand>', '<LocalDebuggerCommand>%LauncherExe%</LocalDebuggerCommand>' | Out-File Plugins\CaptionMod\CaptionMod.vcxproj.user"
call powershell -Command "(gc Plugins\CaptionMod\CaptionMod.vcxproj.user) -replace '<LocalDebuggerCommandArguments>.*</LocalDebuggerCommandArguments>', '<LocalDebuggerCommandArguments>-game %LauncherMod%</LocalDebuggerCommandArguments>' | Out-File Plugins\CaptionMod\CaptionMod.vcxproj.user"
call powershell -Command "(gc Plugins\CaptionMod\CaptionMod.vcxproj.user) -replace '<LocalDebuggerWorkingDirectory>.*</LocalDebuggerWorkingDirectory>', '<LocalDebuggerWorkingDirectory>%GameDir%\</LocalDebuggerWorkingDirectory>' | Out-File Plugins\CaptionMod\CaptionMod.vcxproj.user"

if not exist "Plugins\Renderer\Renderer.vcxproj.user" copy "vcxproj.user.template" "Plugins\Renderer\Renderer.vcxproj.user"

call powershell -Command "(gc Plugins\Renderer\Renderer.vcxproj.user) -replace '<LocalDebuggerCommand>.*</LocalDebuggerCommand>', '<LocalDebuggerCommand>%LauncherExe%</LocalDebuggerCommand>' | Out-File Plugins\Renderer\Renderer.vcxproj.user"
call powershell -Command "(gc Plugins\Renderer\Renderer.vcxproj.user) -replace '<LocalDebuggerCommandArguments>.*</LocalDebuggerCommandArguments>', '<LocalDebuggerCommandArguments>-game %LauncherMod%</LocalDebuggerCommandArguments>' | Out-File Plugins\Renderer\Renderer.vcxproj.user"
call powershell -Command "(gc Plugins\Renderer\Renderer.vcxproj.user) -replace '<LocalDebuggerWorkingDirectory>.*</LocalDebuggerWorkingDirectory>', '<LocalDebuggerWorkingDirectory>%GameDir%\</LocalDebuggerWorkingDirectory>' | Out-File Plugins\Renderer\Renderer.vcxproj.user"

if not exist "Plugins\StudioEvents\StudioEvents.vcxproj.user" copy "vcxproj.user.template" "Plugins\StudioEvents\StudioEvents.vcxproj.user"

call powershell -Command "(gc Plugins\StudioEvents\StudioEvents.vcxproj.user) -replace '<LocalDebuggerCommand>.*</LocalDebuggerCommand>', '<LocalDebuggerCommand>%LauncherExe%</LocalDebuggerCommand>' | Out-File Plugins\StudioEvents\StudioEvents.vcxproj.user"
call powershell -Command "(gc Plugins\StudioEvents\StudioEvents.vcxproj.user) -replace '<LocalDebuggerCommandArguments>.*</LocalDebuggerCommandArguments>', '<LocalDebuggerCommandArguments>-game %LauncherMod%</LocalDebuggerCommandArguments>' | Out-File Plugins\StudioEvents\StudioEvents.vcxproj.user"
call powershell -Command "(gc Plugins\StudioEvents\StudioEvents.vcxproj.user) -replace '<LocalDebuggerWorkingDirectory>.*</LocalDebuggerWorkingDirectory>', '<LocalDebuggerWorkingDirectory>%GameDir%\</LocalDebuggerWorkingDirectory>' | Out-File Plugins\StudioEvents\StudioEvents.vcxproj.user"

echo -----------------------------------------------------

echo All files copied to "%GameDir%"
pause
exit

:fail

echo Failed to locate GameInstallDir of Half-Life, please make sure Steam is running and you have Half-Life installed correctly.
pause
exit