Add-Type -LiteralPath "$(Get-Location)\Indieteur.SAMAPI.dll"
try{
$SAM = New-Object Indieteur.SAMAPI.SteamAppsManager
if ($args.Count -le 0){
    foreach($sapp in $SAM.SteamApps){
        Write-Host "$($sapp.AppID) `"$($sapp.Name)`" `"$($sapp.ProcessNameToFind)`" `"$($sapp.InstallDirName)`" `"$($sapp.InstallDir)`""
    }
}
else{
$appId = $args[0] -as [int]
    foreach($sapp in $SAM.SteamApps){
        if($sapp.AppID -eq $appID){
            if($args.Count -ge 2){
                switch($args[1]){
                    "Name" {Write-Host "$($sapp.Name)";break}
                    "ProcessName" {Write-Host "$($sapp.ProcessNameToFind)";break}
                    "InstallDirName" {Write-Host "$($sapp.InstallDirName)";break}
                    "InstallDir" {Write-Host "$($sapp.InstallDir)";break}
                    Default {Write-Host "arg: $($args[1]) is invalid";break}
                }
                continue
            }
            Write-Host "$($sapp.AppID) `"$($sapp.Name)`" `"$($sapp.ProcessNameToFind)`" `"$($sapp.InstallDirName)`" `"$($sapp.InstallDir)`""
        }
    }
}
}
catch [System.NullReferenceException]{
    [System.Windows.MessageBox]::Show("Steam installation folder was not found or is invalid! Please provide the path to the Steam installation folder.", "Steam App Manager", "OK", "Error")
}