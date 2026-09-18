[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$EngineRoot,[string]$Python='python',[switch]$Package,[switch]$City)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$Root=Split-Path $PSScriptRoot -Parent
$Project=Join-Path $Root 'WroclawTheGame.uproject'
& (Join-Path $PSScriptRoot 'Assert-UnrealVersion.ps1') -EngineRoot $EngineRoot -Project $Project -ExpectedVersion '5.8'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
# Dependency installation is explicit and separate: python -m pip install -r Scripts/gis/requirements.txt
if($City){
    & $Python (Join-Path $PSScriptRoot 'gis\build_city.py') --meshes
    if($LASTEXITCODE -ne 0){throw 'City GIS import failed'}
    # Preserve the existing laboratory trials in their original coordinate frame.
    & $Python (Join-Path $PSScriptRoot 'gis\import_sector.py')
    if($LASTEXITCODE -ne 0){throw 'Existing GIS import failed'}
    & $Python (Join-Path $PSScriptRoot 'gis\make_routes.py')
    if($LASTEXITCODE -ne 0){throw 'Race route preparation failed'}
}else{
    & $Python (Join-Path $PSScriptRoot 'gis\build_meshes.py')
    if($LASTEXITCODE -ne 0){throw 'GIS import failed'}
}
& (Join-Path $PSScriptRoot 'Build-Windows.ps1') -EngineRoot $EngineRoot -PrepareOnly
if(-not $?){throw 'Campaign content preparation failed'}
$Marker=Join-Path $Root 'Saved\GeographyReady.ok'
if(Test-Path $Marker){Remove-Item $Marker}
$Script=Join-Path $PSScriptRoot 'prepare_geography.py'
$PreviousCityInput=[Environment]::GetEnvironmentVariable('WTG_CITY_INPUT')
try{
    if($City){$env:WTG_CITY_INPUT=Join-Path $Root 'Saved\CityData'}
    else{[Environment]::SetEnvironmentVariable('WTG_CITY_INPUT',$null)}
    & $Editor $Project "-ExecutePythonScript=$Script" -unattended -nullrhi -stdout -FullStdOutLogOutput
    if($LASTEXITCODE -ne 0 -or -not(Test-Path $Marker)){throw 'GIS editor generation failed'}
}finally{
    [Environment]::SetEnvironmentVariable('WTG_CITY_INPUT',$PreviousCityInput)
}
& $Editor $Project -run=WorldPartitionConvertCommandlet /Game/Maps/Nadodrze_GIS -SCCProvider=None -AllowCommandletRendering -unattended
if($LASTEXITCODE -ne 0){throw 'GIS World Partition conversion failed'}
$PreviousVerifyMap=[Environment]::GetEnvironmentVariable('WTG_VERIFY_MAP')
$PartitionMarker=Join-Path $Root 'Saved\PartitionVerified.ok'
if(Test-Path $PartitionMarker){Remove-Item $PartitionMarker}
try{
    $env:WTG_VERIFY_MAP='/Game/Maps/Nadodrze_GIS'
    $VerifyScript=Join-Path $PSScriptRoot 'verify_partition.py'
    & $Editor $Project "-ExecutePythonScript=$VerifyScript" -unattended -nullrhi -stdout -FullStdOutLogOutput
    if($LASTEXITCODE -ne 0 -or -not(Test-Path $PartitionMarker)){throw 'GIS streaming verification failed'}
}finally{
    [Environment]::SetEnvironmentVariable('WTG_VERIFY_MAP',$PreviousVerifyMap)
}
& $Editor $Project /Game/Maps/Nadodrze_GIS -run=WorldPartitionBuilderCommandlet -Builder=WorldPartitionHLODsBuilder -SetupHLODs -BuildHLODs -AllowCommandletRendering -SCCProvider=None -unattended
if($LASTEXITCODE -ne 0){throw 'GIS HLOD generation failed'}
if($Package){
    $UAT=Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
    & $UAT BuildCookRun "-project=$Project" -noP4 -platform=Win64 -clientconfig=Development -build -cook '-map=/Game/Maps/Przebudzenie_Source+/Game/Maps/Nadodrze_GIS' -stage -pak -archive "-archivedirectory=$Root\Builds\Geography" -prereqs -utf8output -unattended
    if($LASTEXITCODE -ne 0){throw 'GIS package failed'}
}
