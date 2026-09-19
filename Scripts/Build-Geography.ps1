[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [string]$Python='python',
    [switch]$Package,
    [switch]$City,
    [switch]$Campaign,
    [switch]$OfficialBuildings,
    [string]$OfficialBuildingsArchive
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$Root=Split-Path $PSScriptRoot -Parent
$Project=Join-Path $Root 'WroclawTheGame.uproject'
if($Campaign -and -not $City){throw '-Campaign requires -City because campaign anchors resolve against the combined city GIS dataset.'}
if($OfficialBuildingsArchive){$OfficialBuildings=$true}
if($OfficialBuildings -and -not $City){throw '-OfficialBuildings requires -City.'}
& (Join-Path $PSScriptRoot 'Assert-UnrealVersion.ps1') -EngineRoot $EngineRoot -Project $Project -ExpectedVersion '5.8'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'

if($City){
    $CityData=Join-Path $Root 'Saved\CityData'
    if($OfficialBuildings){
        $CityData=Join-Path $Root 'Saved\CityData'
        $OfficialData=Join-Path $Root 'Saved\OfficialBuildings3D'
        & $Python (Join-Path $PSScriptRoot 'gis\build_city.py') --output $CityData
        if($LASTEXITCODE -ne 0){throw 'City GIS preparation for official buildings failed'}
        $OfficialArgs=@('--city-data',$CityData,'--output',$OfficialData)
        if($OfficialBuildingsArchive){$OfficialArgs+=@('--archive',$OfficialBuildingsArchive)}
        & $Python (Join-Path $PSScriptRoot 'gis\fetch_official_city_buildings.py') @OfficialArgs
        if($LASTEXITCODE -ne 0){throw 'Official Wroclaw city building download/conversion failed'}
        & $Python (Join-Path $PSScriptRoot 'gis\build_city.py') --output $CityData --meshes --official-catalog (Join-Path $OfficialData 'catalog.json')
        if($LASTEXITCODE -ne 0){throw 'City GIS mesh build with official buildings failed'}
    }else{
        & $Python (Join-Path $PSScriptRoot 'gis\build_city.py') --meshes
        if($LASTEXITCODE -ne 0){throw 'City GIS import failed'}
    }
    $SectorInput=Join-Path $CityData 'sector.json'
    $EntrancesOutput=Join-Path $CityData 'entrances.json'
    $FacadesOutput=Join-Path $CityData 'facades.json'
    $RoofDetailsOutput=Join-Path $CityData 'roof_details.json'
    & $Python (Join-Path $PSScriptRoot 'gis\resolve_building_entrances.py') --city $SectorInput --output $EntrancesOutput
    if($LASTEXITCODE -ne 0){throw 'Building entrance resolution failed'}
    & $Python (Join-Path $PSScriptRoot 'gis\generate_facades.py') --city $SectorInput --entrances $EntrancesOutput --output $FacadesOutput
    if($LASTEXITCODE -ne 0){throw 'Facade opening/detail generation failed'}
    & $Python (Join-Path $PSScriptRoot 'gis\generate_roof_details.py') --city $SectorInput --output $RoofDetailsOutput
    if($LASTEXITCODE -ne 0){throw 'Roof detail generation failed'}
    & $Python (Join-Path $PSScriptRoot 'gis\import_sector.py')
    if($LASTEXITCODE -ne 0){throw 'Existing GIS import failed'}
    & $Python (Join-Path $PSScriptRoot 'gis\migrate_campaign.py') --input (Join-Path $Root 'Saved\CityData') --output (Join-Path $Root 'Saved\CampaignGIS')
    if($LASTEXITCODE -ne 0){throw 'Campaign GIS migration data generation failed'}
}else{
    & $Python (Join-Path $PSScriptRoot 'gis\build_meshes.py')
    if($LASTEXITCODE -ne 0){throw 'GIS import failed'}
}
& $Python (Join-Path $PSScriptRoot 'gis\make_routes.py')
if($LASTEXITCODE -ne 0){throw 'Race route preparation failed'}

$PreviousChapterInput=[Environment]::GetEnvironmentVariable('WTG_CHAPTER_INPUT')
$PreviousWorldInput=[Environment]::GetEnvironmentVariable('WTG_WORLD_INPUT')
$PreviousEnvironmentInput=[Environment]::GetEnvironmentVariable('WTG_ENVIRONMENT_INPUT')
try{
    if($Campaign){
        $CampaignDir=Join-Path $Root 'Saved\CampaignGIS'
        $env:WTG_CHAPTER_INPUT=Join-Path $CampaignDir 'chapter1.json'
        $env:WTG_WORLD_INPUT=Join-Path $CampaignDir 'openworld.json'
        $env:WTG_ENVIRONMENT_INPUT=Join-Path $CampaignDir 'environment.json'
    }else{
        [Environment]::SetEnvironmentVariable('WTG_CHAPTER_INPUT',$null)
        [Environment]::SetEnvironmentVariable('WTG_WORLD_INPUT',$null)
        [Environment]::SetEnvironmentVariable('WTG_ENVIRONMENT_INPUT',$null)
    }
    & (Join-Path $PSScriptRoot 'Build-Windows.ps1') -EngineRoot $EngineRoot -PrepareOnly
    if(-not $?){throw 'Campaign content preparation failed'}
}finally{
    [Environment]::SetEnvironmentVariable('WTG_CHAPTER_INPUT',$PreviousChapterInput)
    [Environment]::SetEnvironmentVariable('WTG_WORLD_INPUT',$PreviousWorldInput)
    [Environment]::SetEnvironmentVariable('WTG_ENVIRONMENT_INPUT',$PreviousEnvironmentInput)
}

$Marker=Join-Path $Root 'Saved\GeographyReady.ok'
if(Test-Path $Marker){Remove-Item $Marker}
$Script=Join-Path $PSScriptRoot 'prepare_geography.py'
$PreviousCityInput=[Environment]::GetEnvironmentVariable('WTG_CITY_INPUT')
$PreviousCampaignInput=[Environment]::GetEnvironmentVariable('WTG_CAMPAIGN_GIS_INPUT')
$PreviousOfficialBuildingsInput=[Environment]::GetEnvironmentVariable('WTG_OFFICIAL_BUILDINGS_INPUT')
try{
    if($City){$env:WTG_CITY_INPUT=Join-Path $Root 'Saved\CityData'}
    else{[Environment]::SetEnvironmentVariable('WTG_CITY_INPUT',$null)}
    if($Campaign){$env:WTG_CAMPAIGN_GIS_INPUT=Join-Path $Root 'Saved\CampaignGIS'}
    else{[Environment]::SetEnvironmentVariable('WTG_CAMPAIGN_GIS_INPUT',$null)}
    if($OfficialBuildings){$env:WTG_OFFICIAL_BUILDINGS_INPUT=Join-Path $Root 'Saved\OfficialBuildings3D'}
    else{[Environment]::SetEnvironmentVariable('WTG_OFFICIAL_BUILDINGS_INPUT',$null)}
    & $Editor $Project "-ExecutePythonScript=$Script" -unattended -nullrhi -stdout -FullStdOutLogOutput
    if($LASTEXITCODE -ne 0 -or -not(Test-Path $Marker)){throw 'GIS editor generation failed'}
}finally{
    [Environment]::SetEnvironmentVariable('WTG_CITY_INPUT',$PreviousCityInput)
    [Environment]::SetEnvironmentVariable('WTG_CAMPAIGN_GIS_INPUT',$PreviousCampaignInput)
    [Environment]::SetEnvironmentVariable('WTG_OFFICIAL_BUILDINGS_INPUT',$PreviousOfficialBuildingsInput)
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
    $Archive=Join-Path $Root ($Campaign ? 'Builds\FullGame' : 'Builds\Geography')
    $ConfigPath=Join-Path $Root 'Config\DefaultEngine.ini'
    $OriginalConfig=Get-Content -LiteralPath $ConfigPath -Raw
    try{
        if($Campaign){
            $FullConfig=$OriginalConfig -replace 'GameDefaultMap=/Game/Maps/Przebudzenie_Source','GameDefaultMap=/Game/Maps/Nadodrze_GIS'
            Set-Content -LiteralPath $ConfigPath -Value $FullConfig -NoNewline
        }
        & $UAT BuildCookRun "-project=$Project" -noP4 -platform=Win64 -clientconfig=Development -build -cook '-map=/Game/Maps/Przebudzenie_Source+/Game/Maps/Nadodrze_GIS' -stage -pak -archive "-archivedirectory=$Archive" -prereqs -utf8output -unattended
        if($LASTEXITCODE -ne 0){throw 'GIS package failed'}
    }finally{
        if($Campaign){Set-Content -LiteralPath $ConfigPath -Value $OriginalConfig -NoNewline}
    }
}
