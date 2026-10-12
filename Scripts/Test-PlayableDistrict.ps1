param([string]$EngineRoot = 'D:\SteamLibrary\UnrealEngine\UE_5.8')
$ErrorActionPreference='Stop'
$Root=Split-Path $PSScriptRoot -Parent
$Project=Join-Path $Root 'WroclawTheGame.uproject'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if(!(Test-Path -LiteralPath $Editor)){throw 'Nie znaleziono Unreal Engine; podaj -EngineRoot.'}
Push-Location $Root
try {
    & $Editor $Project -unattended -nosplash -nullrhi '-ExecCmds=Automation RunTests WTG.District' '-TestExit=Automation Test Queue Empty' "-abslog=$Root/Saved/Logs/DistrictRulesTest.log" *> Saved/Logs/DistrictRulesTest-stdout.log
    if($LASTEXITCODE -ne 0){throw 'Native tests process failed'}
    $Log=Get-Content -LiteralPath 'Saved/Logs/DistrictRulesTest.log' -Raw
    foreach($Name in @('DeliveryRules','GeneratedMeshCollision','SaveValidation','WantedDecay')){
        if($Log -notmatch ("Result=\{Success\} Name=\{"+$Name+"\}")){throw "Native test failed: $Name"}
    }
    foreach($Run in @(
        @{Name='DistrictSaveSmoke';Command='WTG.DistrictSaveSmoke';Marker='WTG_DISTRICT_SAVE_DONE result=PASS';Extra=@()},
        @{Name='DistrictSessionSmoke';Command='WTG.DistrictSessionSmoke';Marker='WTG_DISTRICT_SESSION_DONE result=PASS resume=0';Extra=@()},
        @{Name='DistrictSessionReplay';Command='WTG.DistrictSessionSmoke';Marker='WTG_DISTRICT_SESSION_DONE result=PASS resume=1';Extra=@('-WTGDistrictSessionResume')}
    )){
        $Extra=$Run.Extra
        & $Editor $Project /Game/Maps/Nadodrze_Playable -game -unattended -nosplash -nullrhi -WTGNoDistrictAutoLoad -WTGNoDistrictAutoSave "-ExecCmds=$($Run.Command)" "-abslog=$Root/Saved/Logs/$($Run.Name).log" @Extra *> "Saved/Logs/$($Run.Name)-stdout.log"
        if($LASTEXITCODE -ne 0 -or (Get-Content -LiteralPath "Saved/Logs/$($Run.Name).log" -Raw) -notmatch [regex]::Escape($Run.Marker)){throw "Runtime check failed: $($Run.Name)"}
    }
    Write-Output 'PASS: reguły, zapis, pełna sesja i odczyt w nowym procesie.'
}finally{Pop-Location}
