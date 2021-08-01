set pscmdline='call powershell -File %~dp0SteamAppsLocation.ps1 228980 InstallDir'
for /f "delims=" %%a in (%pscmdline%) do set GameName=%%a
echo "%GameName%"
