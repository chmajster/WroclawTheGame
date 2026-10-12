[CmdletBinding()]
param([string]$EngineRoot='D:\SteamLibrary\UnrealEngine\UE_5.8',
      [int]$Width=1280,[int]$Height=720,[string]$ProjectRoot='')
$ErrorActionPreference='Stop'
$taskRoot=if($ProjectRoot){[IO.Path]::GetFullPath($ProjectRoot)}else{Split-Path $PSScriptRoot -Parent}
$editor=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$project=Join-Path $taskRoot 'WroclawTheGame.uproject'
$report=Join-Path $taskRoot 'Saved/StartLocation/gameplay-report.json'
$log=Join-Path $taskRoot 'Saved/StartLocation/StartGameplay.log'
if(!(Test-Path -LiteralPath $editor)){throw 'Unreal executable missing; provide EngineRoot.'}
$started=[DateTime]::UtcNow
Push-Location $taskRoot
try {
    & $editor $project /Game/Maps/Nadodrze_Playable -game -unattended -nosplash -windowed -ForceRes "-ResX=$Width" "-ResY=$Height" -WTGNoFocusPause -WTGNoDistrictAutoLoad -WTGNoDistrictAutoSave '-ExecCmds=WTG.StartLocationSmoke' "-abslog=$log" *> Saved/StartLocation/StartGameplay-stdout.log
    $nativeExit=$LASTEXITCODE
    if($nativeExit -ne 0 -or !(Test-Path -LiteralPath $report) -or (Get-Item -LiteralPath $report).LastWriteTimeUtc -lt $started){throw 'Fresh native gameplay report missing or process failed.'}
    $result=Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
    $text=Get-Content -LiteralPath $log -Raw
    if($result.status -ne 'PASS' -or $text -notmatch 'WTG_START_GAME_DONE result=PASS' -or $text -match 'Fatal error:|LogPython: Error|Usage flag .*missing'){throw 'Actual start gameplay check failed; inspect Saved/StartLocation/StartGameplay.log.'}
    foreach($name in @('Start_GamePlayer','Start_GameWalk','Start_GameStreet','Start_GamePark')){
        $path=Join-Path $taskRoot "Saved/Tests/$name.png"
        if(!(Test-Path -LiteralPath $path) -or (Get-Item -LiteralPath $path).LastWriteTimeUtc -lt $started){throw "Fresh screenshot missing: $name"}
    }
    if($result.width -ne $Width -or $result.height -ne $Height){throw 'Actual viewport size does not match request.'}
    Write-Output ($result | ConvertTo-Json -Depth 5)
}finally{Pop-Location}
