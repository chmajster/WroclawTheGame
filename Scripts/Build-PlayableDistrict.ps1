[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [Parameter(Mandatory=$true)][string]$Blender,
    [string]$Python='python',
    [switch]$Launch
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$Root=Split-Path $PSScriptRoot -Parent
$Project=Join-Path $Root 'WroclawTheGame.uproject'
& (Join-Path $PSScriptRoot 'Assert-UnrealVersion.ps1') -EngineRoot $EngineRoot -Project $Project -ExpectedVersion '5.8'
Push-Location $Root
try {
    & $Python 'Scripts/gis/build_playable_district.py'
    if($LASTEXITCODE -ne 0){throw 'District source generation failed'}
    & $Python -m unittest Tests.test_playable_district
    if($LASTEXITCODE -ne 0){throw 'District geometry checks failed'}
    & $Python 'Pipeline/qa/validate_manifest.py' 'Pipeline/assets/nadodrze/river-railing.asset.json'
    if($LASTEXITCODE -ne 0){throw 'Railing manifest validation failed'}
    & $Blender --background --python 'Pipeline/blender/build_asset.py' -- --manifest 'Pipeline/assets/nadodrze/river-railing.asset.json'
    if($LASTEXITCODE -ne 0){throw 'Blender asset generation failed'}
    & (Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat') WroclawTheGameEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReloadFromIDE
    if($LASTEXITCODE -ne 0){throw 'Editor module compilation failed'}
    if(!(Test-Path 'Saved/FreeModelImportMap.json')){
        throw 'Import the repository free model catalog first (Scripts/import_free_models.py).'
    }
    $Editor=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
    & $Editor $Project "-ExecutePythonScript=$Root/Scripts/prepare_playable_district.py" -unattended -nosplash -nullrhi '-ExecCmds=Interchange.FeatureFlags.Import.FBX 0' "-abslog=$Root/Saved/Logs/DistrictBake.log"
    if($LASTEXITCODE -ne 0){throw 'Unreal district generation process failed; inspect Saved/Logs/DistrictBake.log'}
    if(!(Test-Path 'Saved/PlayableDistrictReady.ok')){throw 'District generation did not complete'}
    if($Launch){
        Start-Process (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe') -ArgumentList @("`"$Project`"",'/Game/Maps/Nadodrze_Playable','-game','-windowed','-ResX=1600','-ResY=900','-nosplash')
    }
}finally{Pop-Location}
