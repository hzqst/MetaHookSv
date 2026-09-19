$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../../..").Path
$vs = & "$repo/tools/vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$msbuild = "$vs/MSBuild/Current/Bin/MSBuild.exe"
$metadata = & $msbuild "$repo/Plugins/Renderer/Renderer.vcxproj" /p:Configuration=Release /p:Platform=Win32 "/p:SolutionDir=$repo\" /getItem:ClCompile
if ($LASTEXITCODE -ne 0) { throw 'Could not evaluate Renderer compiler settings.' }
$settings = ($metadata | ConvertFrom-Json).Items.ClCompile[0]
$testDir = Join-Path ([System.IO.Path]::GetTempPath()) ('renderer-studio-tests-' + [guid]::NewGuid())
New-Item -ItemType Directory -Path $testDir | Out-Null
$flags = @('/nologo', '/std:c++20', '/EHsc', '/MT', '/O1', '/Gy', '/wd4996', ('/I"' + "$repo/Plugins/Renderer" + '"'))
foreach ($include in $settings.AdditionalIncludeDirectories.Split(';')) {
    if ($include -and !$include.Contains('%(')) { $flags += '/I"' + $include + '"' }
}
foreach ($define in $settings.PreprocessorDefinitions.Split(';')) {
    if ($define -and $define -ne 'NDEBUG' -and !$define.Contains('%(')) { $flags += '/D' + $define }
}
foreach ($test in @('studio_model_validation_tests', 'studio_model_load_tests')) {
    $exe = Join-Path $testDir "$test.exe"
    $rsp = Join-Path $testDir "$test.rsp"
    $argsForCompiler = $flags + @('"' + "$PSScriptRoot/$test.cpp" + '"', '/Fo"' + "$testDir/$test.obj" + '"', '/Fe"' + $exe + '"', '/link', '/OPT:REF')
    [System.IO.File]::WriteAllLines($rsp, $argsForCompiler)
    $batch = Join-Path $testDir "$test.bat"
    [System.IO.File]::WriteAllLines($batch, @('@echo off', "call `"$vs/VC/Auxiliary/Build/vcvars32.bat`" >nul", "cl @`"$rsp`""))
    & $env:ComSpec /d /c $batch
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $test" }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw "Test failed: $test" }
}
Write-Host "Test artifacts: $testDir"
