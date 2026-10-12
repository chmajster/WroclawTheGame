param([string]$EngineRoot='D:\SteamLibrary\UnrealEngine\UE_5.8', [switch]$Layouts)
$ErrorActionPreference='Stop'
$Root=Split-Path $PSScriptRoot -Parent
$Editor=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$Commandlet=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$Project=Join-Path $Root 'WroclawTheGame.uproject'
New-Item -ItemType Directory -Force -Path "$Root/Saved/MenuQA","$Root/Saved/Tests" | Out-Null
function Run-MenuProbe([string]$Name,[string]$Command,[string]$Marker,[string[]]$Extra=@(),[bool]$Rendered=$false) {
    $Executable=if($Rendered){$Editor}else{$Commandlet}
    $Log="$Root/Saved/Logs/$Name.log"
    $Started=[DateTime]::UtcNow
    $Arguments=@(('"{0}"' -f $Project),'/Game/Maps/Nadodrze_Playable','-game','-unattended','-nosplash',
        '-WTGMenuTest','-WTGNoDistrictAutoLoad','-WTGNoDistrictAutoSave',
        ('-ExecCmds="DisableAllScreenMessages,{0}"' -f $Command),
        ('-GameUserSettingsINI="{0}/Saved/MenuQA/{1}.ini"' -f $Root,$Name),('-abslog="{0}"' -f $Log))
    if($Rendered){$Arguments+='-windowed';if(!($Extra -match '^-ResX=')){$Arguments+=@('-ResX=1600','-ResY=900')}}else{$Arguments+='-nullrhi'}
    $Arguments+=$Extra
    $Process=Start-Process -FilePath $Executable -WorkingDirectory $Root -ArgumentList $Arguments -WindowStyle Hidden -PassThru
    $Deadline=[DateTime]::UtcNow.AddSeconds(150)
    while(!$Process.HasExited -and [DateTime]::UtcNow -lt $Deadline){Start-Sleep -Seconds 1;$Process.Refresh()}
    if(!$Process.HasExited){Stop-Process -Id $Process.Id;throw "Timeout: $Name (only owned probe stopped)"}
    $Output=Get-Content -LiteralPath $Log -Raw
    if((Get-Item -LiteralPath $Log).LastWriteTimeUtc -lt $Started -or $Process.ExitCode -ne 0 -or !$Output.Contains($Marker) -or $Output -match 'Error:|Fatal error:') {throw "Failed: $Name; inspect $Log"}
    Write-Output "PASS: $Name"
}
Run-MenuProbe 'MenuSettingsQA' 'WTG.MenuSettingsSmoke' 'WTG_MENU_SETTINGS_DONE result=PASS replay=0'
# Replay must use the same INI in a separate process, without running setters again.
& $Commandlet $Project /Game/Maps/Nadodrze_Playable -game -nullrhi -unattended -nosplash -WTGMenuTest -WTGMenuSettingsReplay -WTGNoDistrictAutoLoad -WTGNoDistrictAutoSave '-ExecCmds=WTG.MenuSettingsSmoke' "-GameUserSettingsINI=$Root/Saved/MenuQA/MenuSettingsQA.ini" "-abslog=$Root/Saved/Logs/MenuSettingsQAReplay.log" *> "$Root/Saved/Logs/MenuSettingsQAReplay.stdout"
if($LASTEXITCODE -ne 0 -or !(Get-Content "$Root/Saved/Logs/MenuSettingsQAReplay.log" -Raw).Contains('WTG_MENU_SETTINGS_DONE result=PASS replay=1')){throw 'Settings replay failed'}
Write-Output 'PASS: settings replay in separate process'
Run-MenuProbe 'MenuUIQA' 'WTG.MenuUISmoke' 'WTG_MENU_UI_DONE result=PASS' @() $true
if($Layouts){
    foreach($Size in @(@{Name='1080';X=1920;Y=1080},@{Name='1440';X=2560;Y=1440},@{Name='4K';X=3840;Y=2160},@{Name='Ultrawide';X=3440;Y=1440})){
        Run-MenuProbe "MenuLayout$($Size.Name)" 'WTG.MenuViewSmoke' 'WTG_MENU_VIEW_DONE result=PASS' @('-ForceRes',"-ResX=$($Size.X)","-ResY=$($Size.Y)") $true
        foreach($Page in @('Main','Pause','Settings','Controls','Loading')){Copy-Item -LiteralPath "$Root/Saved/Tests/Menu$Page.png" -Destination "$Root/Saved/Tests/Menu${Page}_$($Size.Name).png"}
    }
}
Write-Output 'PASS: menu probes. Rendered images: Saved/Tests. No CI invoked.'
