$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../../..").Path
$vs = & "$repo/tools/vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual Studio C++ tools were not found.' }
$testDir = Join-Path ([System.IO.Path]::GetTempPath()) ('vgui2-language-tests-' + [guid]::NewGuid())
New-Item -ItemType Directory -Path $testDir | Out-Null
$exe = Join-Path $testDir 'language_registry_tests.exe'
$rsp = Join-Path $testDir 'tests.rsp'
[System.IO.File]::WriteAllLines($rsp, @('/nologo', '/std:c++17', '/EHsc', '/MT', '/W4', '/WX',
    ('"' + "$PSScriptRoot/language_registry_tests.cpp" + '"'),
    ('/Fo"' + "$testDir/tests.obj" + '"'), ('/Fe"' + $exe + '"')))
$batch = Join-Path $testDir 'build.bat'
[System.IO.File]::WriteAllLines($batch, @('@echo off', "call `"$vs/VC/Auxiliary/Build/vcvars32.bat`" >nul", "cl @`"$rsp`""))
& $env:ComSpec /d /c $batch
if ($LASTEXITCODE -ne 0) { throw 'Language registry test compilation failed.' }
& $exe
if ($LASTEXITCODE -ne 0) { throw 'Language registry tests failed.' }
Write-Host "Test artifacts: $testDir"
