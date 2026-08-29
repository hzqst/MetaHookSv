#Requires -Version 5.1
<#
.SYNOPSIS
    Downloads and atomically installs the MetaHook gamedata catalog.

.DESCRIPTION
    Fetches the fixed HTTPS index.json and every snapshot it declares, verifies
    size and SHA-256, validates the JSON schema, then swaps the completed
    staging directory into the destination using a same-volume transactional
    rename. A named mutex serializes concurrent builds that target the same
    destination.

    With -ValidateOnly the script performs an offline integrity check of the
    existing destination directory and never touches the network.

.PARAMETER IndexUrl
    The fixed HTTPS URL of index.json.

.PARAMETER Destination
    The target gamedata directory (e.g. Build\svencoop\metahook\gamedata).

.PARAMETER ValidateOnly
    Do not download; validate the current destination directory instead.

.PARAMETER TimeoutSec
    HTTP timeout per request.

.PARAMETER RetryCount
    Number of download attempts for transient failures.

.PARAMETER MutexTimeoutSec
    Maximum time to wait for the cross-process sync mutex.

.EXAMPLE
    .\sync-gamedata.ps1 -IndexUrl "https://..." -Destination "Build\svencoop\metahook\gamedata"

.EXAMPLE
    .\sync-gamedata.ps1 -Destination "Build\svencoop\metahook\gamedata" -ValidateOnly
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $false)]
    [string]$IndexUrl = "",

    [Parameter(Mandatory = $false)]
    [string]$Destination = "",

    [switch]$ValidateOnly,

    [int]$TimeoutSec = 120,

    [int]$RetryCount = 3,

    [int]$MutexTimeoutSec = 600
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

# Required in Windows PowerShell 5.1 before constructing System.Net.Http types.
Add-Type -AssemblyName System.Net.Http

$ScriptName = 'sync-gamedata.ps1'

function Write-Log {
    param([string]$Message)
    Write-Host "[$ScriptName] $Message"
}

function Write-Fail {
    param([string]$Message)
    Write-Host "[$ScriptName] ERROR: $Message" -ForegroundColor Red
}

# ---------------------------------------------------------------------------
# Path / URL safety helpers
# ---------------------------------------------------------------------------

function Test-SafeRelativeFilename {
    param([string]$Name)

    if ([string]::IsNullOrWhiteSpace($Name)) { return $false }

    # A single safe relative filename: no path separators, no drive letter,
    # no UNC, no parent traversal, no control characters.
    if ($Name.IndexOfAny([char[]]@('/', '\')) -ge 0) { return $false }
    if ($Name -match '^[a-zA-Z]:') { return $false }
    if ($Name -eq '.' -or $Name -eq '..') { return $false }
    if ($Name -match '^\.\.?[\\/]') { return $false }
    if ($Name.IndexOf([char]0) -ge 0) { return $false }
    if ($Name.IndexOfAny([char[]]@([char]0x00 .. [char]0x1F)) -ge 0) { return $false }

    return $true
}

function Test-LowerHex {
    param([string]$Value, [int]$Length)
    if ($null -eq $Value) { return $false }
    if ($Value.Length -ne $Length) { return $false }
    return $Value -match '^[0-9a-f]+$'
}

function Assert-ValidIndexUrl {
    param([string]$Url)
    if ([string]::IsNullOrWhiteSpace($Url)) {
        throw 'IndexUrl is empty.'
    }
    try {
        $uri = New-Object System.Uri($Url)
    } catch {
        throw "IndexUrl is not a valid URI: $Url"
    }
    if ($uri.Scheme -ne 'https') {
        throw "IndexUrl must use https, got '$($uri.Scheme)'."
    }
    return $uri
}

# ---------------------------------------------------------------------------
# Network helpers
# ---------------------------------------------------------------------------

function Invoke-DownloadFile {
    param(
        [string]$Url,
        [string]$OutPath,
        [int]$TimeoutSec = 120
    )

    $handler = New-Object System.Net.Http.HttpClientHandler
    $handler.AllowAutoRedirect = $true
    $client = New-Object System.Net.Http.HttpClient($handler)
    $client.Timeout = [TimeSpan]::FromSeconds($TimeoutSec)
    try {
        $response = $client.GetAsync(
            $Url,
            [System.Net.Http.HttpCompletionOption]::ResponseHeadersRead
        ).GetAwaiter().GetResult()

        if (-not $response.IsSuccessStatusCode) {
            throw "HTTP $([int]$response.StatusCode) ($($response.ReasonPhrase)) for $Url"
        }

        $stream = $response.Content.ReadAsStreamAsync().GetAwaiter().GetResult()
        $fileStream = [System.IO.File]::Create($OutPath)
        try {
            $stream.CopyTo($fileStream)
        } finally {
            $fileStream.Dispose()
            $stream.Dispose()
        }
    } finally {
        $client.Dispose()
    }
}

function Invoke-DownloadWithRetry {
    param(
        [string]$Url,
        [string]$OutPath,
        [int]$RetryCount = 3,
        [int]$TimeoutSec = 120
    )

    $attempt = 0
    while ($true) {
        $attempt++
        try {
            Invoke-DownloadFile -Url $Url -OutPath $OutPath -TimeoutSec $TimeoutSec
            return
        } catch {
            if ($attempt -ge $RetryCount) { throw }
            $delay = [Math]::Pow(2, $attempt - 1)  # 1s, 2s, 4s, ...
            Write-Log "Download attempt $attempt/$RetryCount failed: $($_.Exception.Message). Retrying in ${delay}s..."
            Start-Sleep -Seconds $delay
        }
    }
}

function Resolve-SnapshotUrl {
    param(
        [System.Uri]$BaseUri,
        [string]$Relative
    )

    $resolved = New-Object System.Uri($BaseUri, $Relative)
    if ($resolved.Scheme -ne 'https') {
        throw "Snapshot URL must use https, got '$($resolved.Scheme)'."
    }
    if ($resolved.Host -ne $BaseUri.Host -or $resolved.Port -ne $BaseUri.Port) {
        throw "Snapshot URL must not change host/port: '$($resolved.AbsoluteUri)'."
    }
    return $resolved
}

function Get-Sha256Hex {
    param([string]$Path)
    return (Get-FileHash -Path $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

# ---------------------------------------------------------------------------
# JSON schema validation
# ---------------------------------------------------------------------------

function Assert-IndexSchema {
    param($Index)

    if ($null -eq $Index) { throw 'index.json is not a valid JSON object.' }

    if ($Index.schemaVersion -ne 4) {
        $got = $Index.schemaVersion
        throw "Unsupported index schemaVersion: expected 4, got '$got'."
    }

    if ($null -eq $Index.versions -or $Index.versions -isnot [System.Array]) {
        throw 'index.json "versions" must be an array.'
    }

    $seen = @{}
    foreach ($v in $Index.versions) {
        if ($null -eq $v.gameVersion -or $v.gameVersion -isnot [string] -or $v.gameVersion.Length -eq 0) {
            throw 'index.json version entry is missing a non-empty "gameVersion".'
        }
        if ($seen.ContainsKey($v.gameVersion)) {
            throw "index.json declares duplicate gameVersion '$($v.gameVersion)'."
        }
        $seen[$v.gameVersion] = $true

        if (-not (Test-SafeRelativeFilename -Name $v.url)) {
            throw "index.json entry '$($v.gameVersion)' has an unsafe url '$($v.url)'."
        }
        if (-not (Test-LowerHex -Value $v.sha256 -Length 64)) {
            throw "index.json entry '$($v.gameVersion)' has an invalid sha256 '$($v.sha256)'."
        }
        if ($v.size -isnot [long] -and $v.size -isnot [int]) {
            throw "index.json entry '$($v.gameVersion)' has a non-integer size."
        }
        if ([long]$v.size -lt 0) {
            throw "index.json entry '$($v.gameVersion)' has a negative size."
        }
        if ($null -eq $v.snapshotSchemaVersion) {
            throw "index.json entry '$($v.gameVersion)' is missing snapshotSchemaVersion."
        }
    }

    return $seen
}

function Assert-SnapshotSchema {
    param($Snapshot, [string]$GameVersion)

    if ($null -eq $Snapshot) {
        throw "snapshot '$GameVersion' is not a valid JSON object."
    }
    if ($Snapshot.schemaVersion -ne 3) {
        $got = $Snapshot.schemaVersion
        throw "snapshot '$GameVersion' has unsupported schemaVersion: expected 3, got '$got'."
    }
    $srcVersion = $Snapshot.source.snapshotSchemaVersion
    if ($srcVersion -ne 6) {
        throw "snapshot '$GameVersion' has unsupported source.snapshotSchemaVersion: expected 6, got '$srcVersion'."
    }
}

# ---------------------------------------------------------------------------
# Directory swap
# ---------------------------------------------------------------------------

function Invoke-TransactionalSwap {
    param(
        [string]$StagingDir,
        [string]$DestDir
    )

    $parent = Split-Path -Parent $DestDir
    if (-not (Test-Path -LiteralPath $parent)) {
        throw "Destination parent does not exist: $parent"
    }

    $backup = Join-Path $parent ('.gamedata.backup.' + $PID + '.' + [guid]::NewGuid().ToString('N'))
    $hadDestination = Test-Path -LiteralPath $DestDir

    try {
        if ($hadDestination) {
            [System.IO.Directory]::Move($DestDir, $backup)
            Write-Log "Moved existing destination to backup: $backup"
        }

        try {
            [System.IO.Directory]::Move($StagingDir, $DestDir)
        } catch {
            # Restore the previous generation immediately.
            if ($hadDestination -and (Test-Path -LiteralPath $backup) -and -not (Test-Path -LiteralPath $DestDir)) {
                [System.IO.Directory]::Move($backup, $DestDir)
                Write-Log 'Staging->destination move failed; restored previous destination.'
            }
            throw
        }

        if ($hadDestination -and (Test-Path -LiteralPath $backup)) {
            [System.IO.Directory]::Delete($backup, $true)
            Write-Log 'Removed previous generation backup.'
        }

        Write-Log "Installed new gamedata generation: $DestDir"
    } catch {
        if ($hadDestination -and (Test-Path -LiteralPath $backup) -and -not (Test-Path -LiteralPath $DestDir)) {
            [System.IO.Directory]::Move($backup, $DestDir)
        }
        throw
    }
}

function Remove-DirIfInside {
    param([string]$Dir, [string]$MustBeParent)

    if ([string]::IsNullOrWhiteSpace($Dir)) { return }
    if (-not (Test-Path -LiteralPath $Dir)) { return }

    $dirFull = [System.IO.Path]::GetFullPath($Dir)
    $parentFull = [System.IO.Path]::GetFullPath($MustBeParent)
    $parentPrefix = $parentFull.TrimEnd('\') + '\'

    if (-not $dirFull.StartsWith($parentPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Log "Refusing to delete directory outside parent: $Dir"
        return
    }
    if ([System.IO.Path]::GetFileName($dirFull.TrimEnd('\')) -notmatch '^\.gamedata\.(staging|backup)\.') {
        Write-Log "Refusing to delete non-staging/backup directory: $Dir"
        return
    }

    [System.IO.Directory]::Delete($dirFull, $true)
    Write-Log "Cleaned up: $dirFull"
}

# ---------------------------------------------------------------------------
# Validation of an existing directory (offline)
# ---------------------------------------------------------------------------

function Test-ExistingDirectory {
    param(
        [string]$DestDir,
        [System.Uri]$BaseUri
    )

    $indexPath = Join-Path $DestDir 'index.json'
    if (-not (Test-Path -LiteralPath $indexPath -PathType Leaf)) {
        throw "index.json is missing from $DestDir"
    }

    $index = Get-Content -LiteralPath $indexPath -Raw | ConvertFrom-Json
    $declared = Assert-IndexSchema -Index $index

    $expectedFiles = @{ 'index.json' = $true }
    foreach ($v in $index.versions) {
        $expectedFiles[$v.url] = $true
    }

    $extra = Get-ChildItem -LiteralPath $DestDir -File |
        Where-Object { -not $expectedFiles.ContainsKey($_.Name) } |
        Select-Object -ExpandProperty Name
    if ($extra) {
        throw "Destination contains files not declared by index.json: $($extra -join ', ')"
    }

    foreach ($v in $index.versions) {
        $snapPath = Join-Path $DestDir $v.url
        if (-not (Test-Path -LiteralPath $snapPath -PathType Leaf)) {
            throw "Snapshot '$($v.gameVersion)' file is missing: $($v.url)"
        }
        $len = (Get-Item -LiteralPath $snapPath).Length
        if ($len -ne [long]$v.size) {
            throw "Snapshot '$($v.gameVersion)' size mismatch: expected $($v.size), got $len."
        }
        $hash = Get-Sha256Hex -Path $snapPath
        if ($hash -ne $v.sha256) {
            throw "Snapshot '$($v.gameVersion)' SHA-256 mismatch: expected $($v.sha256), got $hash."
        }
        $snap = Get-Content -LiteralPath $snapPath -Raw | ConvertFrom-Json
        Assert-SnapshotSchema -Snapshot $snap -GameVersion $v.gameVersion
        Write-Log "Validated snapshot '$($v.gameVersion)' ($($v.size) bytes)."
    }

    Write-Log "Offline validation passed for $DestDir ($($index.versions.Count) snapshots)."
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

function Main {
    if ([string]::IsNullOrWhiteSpace($Destination)) {
        throw 'Destination is required.'
    }

    $destDir = [System.IO.Path]::GetFullPath($Destination)

    if ($ValidateOnly) {
        # Offline validation only needs the local index.json; the URL is used
        # solely to assert the same scheme/host/port if provided.
        if (-not [string]::IsNullOrWhiteSpace($IndexUrl)) {
            $baseUri = Assert-ValidIndexUrl -Url $IndexUrl
        } else {
            $baseUri = $null
        }
        Test-ExistingDirectory -DestDir $destDir -BaseUri $baseUri
        return
    }

    $baseUri = Assert-ValidIndexUrl -Url $IndexUrl

    # --- Cross-process mutex derived from the normalized destination. ---
    $sha = [System.Security.Cryptography.SHA256]::Create()
    $destKeyBytes = $sha.ComputeHash([System.Text.Encoding]::UTF8.GetBytes($destDir.ToLowerInvariant()))
    $mutexName = 'Local\MetaHookGameDataSync_' + (($destKeyBytes | ForEach-Object { $_.ToString('x2') }) -join '')
    $mutex = New-Object System.Threading.Mutex($false, $mutexName)
    $ownsMutex = $false

    try {
        try {
            $ownsMutex = $mutex.WaitOne([TimeSpan]::FromSeconds($MutexTimeoutSec))
        } catch [System.Threading.AbandonedMutexException] {
            $ownsMutex = $true
        }
        if (-not $ownsMutex) {
            throw "Timed out waiting for the gamedata sync mutex after $MutexTimeoutSec seconds."
        }

        $parent = Split-Path -Parent $destDir
        if (-not (Test-Path -LiteralPath $parent)) {
            throw "Destination parent does not exist: $parent"
        }

        $staging = Join-Path $parent ('.gamedata.staging.' + $PID + '.' + [guid]::NewGuid().ToString('N'))
        New-Item -ItemType Directory -Path $staging | Out-Null
        Write-Log "Staging directory: $staging"

        # --- Download index.json ---
        $indexLocal = Join-Path $staging 'index.json'
        Invoke-DownloadWithRetry -Url $baseUri.AbsoluteUri -OutPath $indexLocal `
            -RetryCount $RetryCount -TimeoutSec $TimeoutSec
        Write-Log 'Downloaded index.json.'

        $index = Get-Content -LiteralPath $indexLocal -Raw | ConvertFrom-Json
        Assert-IndexSchema -Index $index | Out-Null
        Write-Log "index.json declares $($index.versions.Count) snapshots."

        $declaredFiles = @{ 'index.json' = $true }
        foreach ($v in $index.versions) {
            $declaredFiles[$v.url] = $true
        }

        # --- Download and verify every snapshot ---
        foreach ($v in $index.versions) {
            $snapUrl = Resolve-SnapshotUrl -BaseUri $baseUri -Relative $v.url
            $snapLocal = Join-Path $staging $v.url
            Invoke-DownloadWithRetry -Url $snapUrl.AbsoluteUri -OutPath $snapLocal `
                -RetryCount $RetryCount -TimeoutSec $TimeoutSec

            $len = (Get-Item -LiteralPath $snapLocal).Length
            if ($len -ne [long]$v.size) {
                throw "Snapshot '$($v.gameVersion)' size mismatch: expected $($v.size), got $len."
            }

            $hash = Get-Sha256Hex -Path $snapLocal
            if ($hash -ne $v.sha256) {
                throw "Snapshot '$($v.gameVersion)' SHA-256 mismatch: expected $($v.sha256), got $hash."
            }

            $snap = Get-Content -LiteralPath $snapLocal -Raw | ConvertFrom-Json
            Assert-SnapshotSchema -Snapshot $snap -GameVersion $v.gameVersion
            Write-Log "Downloaded and verified snapshot '$($v.gameVersion)' ($($v.size) bytes)."
        }

        # --- No undeclared files may exist in staging. ---
        $extra = Get-ChildItem -LiteralPath $staging -File |
            Where-Object { -not $declaredFiles.ContainsKey($_.Name) } |
            Select-Object -ExpandProperty Name
        if ($extra) {
            throw "Staging contains undeclared files: $($extra -join ', ')"
        }

        # --- Atomic swap ---
        Invoke-TransactionalSwap -StagingDir $staging -DestDir $destDir
        Write-Log "Gamedata sync complete."
    } finally {
        if ($ownsMutex) {
            $mutex.ReleaseMutex()
        }
        $mutex.Dispose()
        if (Test-Path Variable:staging) {
            Remove-DirIfInside -Dir $staging -MustBeParent $parent
        }
    }
}

try {
    Main
    exit 0
} catch {
    Write-Fail $_.Exception.Message
    exit 1
}
