[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$EngineRoot,[string]$Python='python',[switch]$Package)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$Root=Split-Path $PSScriptRoot -Parent
$Project=Join-Path $Root 'WroclawTheGame.uproject'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
# Dependency installation is explicit and separate: python -m pip install -r Scripts/gis/requirements.txt
& $Python (Join-Path $PSScriptRoot 'gis\build_meshes.py')
if($LASTEXITCODE -ne 0){throw 'GIS import failed'}
& (Join-Path $PSScriptRoot 'Build-Windows.ps1') -EngineRoot $EngineRoot -PrepareOnly
if(-not $?){throw 'Campaign content preparation failed'}
$Marker=Join-Path $Root 'Saved\GeographyReady.ok'
if(Test-Path $Marker){Remove-Item $Marker}
$Script=Join-Path $PSScriptRoot 'prepare_geography.py'
& $Editor $Project "-ExecutePythonScript=$Script" -unattended -nullrhi -stdout -FullStdOutLogOutput
if($LASTEXITCODE -ne 0 -or -not(Test-Path $Marker)){throw 'GIS editor generation failed'}
& $Editor $Project -run=WorldPartitionConvertCommandlet /Game/Maps/Nadodrze_GIS -SCCProvider=None -AllowCommandletRendering -unattended
if($LASTEXITCODE -ne 0){throw 'GIS World Partition conversion failed'}
& $Editor $Project /Game/Maps/Nadodrze_GIS -run=WorldPartitionBuilderCommandlet -Builder=WorldPartitionHLODsBuilder -SetupHLODs -BuildHLODs -AllowCommandletRendering -SCCProvider=None -unattended
if($LASTEXITCODE -ne 0){throw 'GIS HLOD generation failed'}
if($Package){
    $UAT=Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
    & $UAT BuildCookRun "-project=$Project" -noP4 -platform=Win64 -clientconfig=Development -build -cook '-map=/Game/Maps/Przebudzenie_Source+/Game/Maps/Nadodrze_GIS' -stage -pak -archive "-archivedirectory=$Root\Builds\Geography" -prereqs -utf8output -unattended
    if($LASTEXITCODE -ne 0){throw 'GIS package failed'}
}
