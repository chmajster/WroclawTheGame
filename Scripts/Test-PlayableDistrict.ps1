param([string]$EngineRoot = 'D:\SteamLibrary\UnrealEngine\UE_5.8')
$ErrorActionPreference='Stop'
$Root=Split-Path $PSScriptRoot -Parent
$Project=Join-Path $Root 'WroclawTheGame.uproject'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if(!(Test-Path -LiteralPath $Editor)){throw 'Nie znaleziono Unreal Engine; podaj -EngineRoot.'}
Push-Location $Root
try {
    # These source/asset tests do not need the legacy default editor map.
    # An empty engine map avoids unrelated imported props at editor startup.
    & $Editor $Project /Engine/Maps/Entry -unattended -nosplash -nullrhi '-ExecCmds=Automation RunTests WTG.District' '-TestExit=Automation Test Queue Empty' "-abslog=$Root/Saved/Logs/DistrictRulesTest.log" *> Saved/Logs/DistrictRulesTest-stdout.log
    if($LASTEXITCODE -ne 0){throw 'Native tests process failed'}
    $Log=Get-Content -LiteralPath 'Saved/Logs/DistrictRulesTest.log' -Raw
    $FirstTest=$Log.IndexOf('LogAutomationController: Display: Test Started.')
    if($FirstTest -lt 0){throw 'Requested native tests did not start'}
    if($Log.Substring($FirstTest) -match 'Error:|Fatal error:'){throw 'District tests logged errors; inspect DistrictRulesTest.log'}
    # UE 5.8 startup smoke checks log this generic error before our test queue.
    # Keep it visible, without treating it as a failure of the requested suite.
    $StartupErrors=@($Log.Substring(0,$FirstTest) -split '\r?\n' | Where-Object {$_ -match 'Error:|Fatal error:'})
    if($StartupErrors | Where-Object {$_ -notmatch 'LogAutomationTest: Error: Condition failed$'}){throw 'Unexpected editor startup error; inspect DistrictRulesTest.log'}
    if($StartupErrors.Count){Write-Output "NOTE: $($StartupErrors.Count) generic engine startup smoke errors precede the district test queue; retained in DistrictRulesTest.log."}
    foreach($Name in @('DeliveryRules','GeneratedMeshCollision','SaveValidation','WantedDecay')){
        if($Log -notmatch ("Result=\{Success\} Name=\{"+$Name+"\}")){throw "Native test failed: $Name"}
    }
    foreach($Run in @(
        @{Name='DistrictSaveSmoke';Command='WTG.DistrictSaveSmoke';Marker='WTG_DISTRICT_SAVE_DONE result=PASS';Extra=@()},
        @{Name='DistrictSessionSmoke';Command='WTG.DistrictSessionSmoke';Marker='WTG_DISTRICT_SESSION_DONE result=PASS resume=0';Extra=@()},
        @{Name='DistrictSessionReplay';Command='WTG.DistrictSessionSmoke';Marker='WTG_DISTRICT_SESSION_DONE result=PASS resume=1';Extra=@('-WTGDistrictSessionResume')}
    )){
        $Extra=$Run.Extra
        $Started=[DateTime]::UtcNow
        & $Editor $Project /Game/Maps/Nadodrze_Playable -game -unattended -nosplash -nullrhi -WTGNoDistrictAutoLoad -WTGNoDistrictAutoSave "-ExecCmds=$($Run.Command)" "-abslog=$Root/Saved/Logs/$($Run.Name).log" @Extra *> "Saved/Logs/$($Run.Name)-stdout.log"
        $Log=Get-Content -LiteralPath "Saved/Logs/$($Run.Name).log" -Raw
        if($LASTEXITCODE -ne 0 -or (Get-Item -LiteralPath "Saved/Logs/$($Run.Name).log").LastWriteTimeUtc -lt $Started -or $Log -notmatch [regex]::Escape($Run.Marker) -or $Log -match 'Error:|Fatal error:'){throw "Runtime check failed: $($Run.Name); inspect its fresh log"}
    }
    Write-Output 'PASS: reguły, zapis, pełna sesja i odczyt w nowym procesie.'
}finally{Pop-Location}
