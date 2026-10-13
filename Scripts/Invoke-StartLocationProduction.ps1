[CmdletBinding()]
param(
 [ValidateSet('Validate','Source','Full')][string]$Mode='Validate',
 [string]$EngineRoot='D:\SteamLibrary\UnrealEngine\UE_5.8',
 [string]$BlenderExe='C:\Program Files\Blender Foundation\Blender 5.2\blender.exe',
 [string]$PythonExe='',[switch]$DownloadReviewed,[switch]$GameplayQA)
$ErrorActionPreference='Stop'
$taskRoot=Split-Path $PSScriptRoot -Parent
if(!$PythonExe){$candidate=Join-Path $taskRoot 'Saved/GISPython/Scripts/python.exe';$PythonExe=if(Test-Path -LiteralPath $candidate){$candidate}else{'python'}}
$out=Join-Path $taskRoot 'Saved/StartLocation/production'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$steps=[Collections.Generic.List[object]]::new()
$editor=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$project=Join-Path $taskRoot 'WroclawTheGame.uproject'
$previousBatch=$env:WTG_ASSET_MANIFESTS;$previousManifest=$env:WTG_ASSET_MANIFEST
function Save-State([string]$Status){
 [ordered]@{mode=$Mode;status=$Status;updated_utc=[DateTime]::UtcNow.ToString('o');steps=@($steps.ToArray());
   note='PASS certifies only the selected mode and recorded executed stages; captures require separate visual inspection.'} |
  ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $out 'report.json') -Encoding UTF8
}
function Invoke-Step([string]$Name,[scriptblock]$Action){
 Write-Output "START $Name"
 $steps.Add([ordered]@{name=$Name;status='RUNNING'});Save-State 'RUNNING'
 & $Action
 if($LASTEXITCODE -ne 0){throw "$Name failed with native exit $LASTEXITCODE"}
 $steps[$steps.Count-1].status='PASS';Save-State 'RUNNING';Write-Output "PASS $Name"
}
function Invoke-Editor([string]$Name,[string]$Script,[string]$Marker,[switch]$Render){
 $started=[DateTime]::UtcNow;$log=Join-Path $out "$Name.log"
 $editorArgs=@($project,'/Engine/Maps/Entry','-unattended','-nosplash',"-ExecutePythonScript=$(Join-Path $taskRoot $Script)","-abslog=$log")
 if(!$Render){$editorArgs+='-nullrhi'}
 & $editor @editorArgs *> (Join-Path $out "$Name-stdout.log")
 $nativeExit=$LASTEXITCODE;$path=Join-Path $taskRoot $Marker
 if($nativeExit -ne 0 -or !(Test-Path -LiteralPath $path) -or (Get-Item -LiteralPath $path).LastWriteTimeUtc -lt $started){throw "$Name failed or lacks fresh marker; inspect $log"}
 if(Select-String -LiteralPath $log -Pattern 'LogPython: Error|Fatal error:|Usage flag .*missing'){throw "$Name logged an error; inspect $log"}
}
Push-Location $taskRoot
try {
 if($Mode -ne 'Validate'){
  $branch=& git branch --show-current
  if($LASTEXITCODE -ne 0 -or !$branch -or $branch -eq 'main'){throw 'Create a dedicated branch from main before producing versioned source/native assets.'}
 }
 $acquire=@('Scripts/assets/acquire_city_assets.py');if($DownloadReviewed){$acquire+='--download'}
 Invoke-Step 'reviewed-source-checksums' {& $PythonExe @acquire}
 if($Mode -eq 'Validate'){
  Invoke-Step 'manifest-validation' {& $PythonExe Pipeline/qa/validate_manifest.py --all}
 }else{
  Invoke-Step 'manifest-schema' {& $PythonExe Pipeline/qa/validate_manifest.py --all --schema-only}
 }
 if($Mode -ne 'Validate'){
  Invoke-Step 'source-gis' {& $PythonExe Scripts/gis/build_playable_district.py}
  Invoke-Step 'start-audit' {& $PythonExe Scripts/assets/audit_start_location.py}
  Invoke-Step 'source-footprints' {& $PythonExe Scripts/assets/build_start_location_inputs.py}
  Invoke-Step 'foliage-source-textures' {& $PythonExe Scripts/assets/build_foliage_textures.py}
  Invoke-Step 'original-blender-models' {& $BlenderExe --background --python-exit-code 1 --python Scripts/assets/build_start_location_models.py *> (Join-Path $out 'models.log')}
  Invoke-Step 'street-infrastructure-models' {& $BlenderExe --background --python-exit-code 1 --python Scripts/assets/build_street_infrastructure.py *> (Join-Path $out 'infrastructure.log')}
  Invoke-Step 'production-manifests' {& $PythonExe Scripts/assets/make_start_location_manifests.py}
  Invoke-Step 'manifest-validation' {& $PythonExe Pipeline/qa/validate_manifest.py --all}
  foreach($manifest in Get-ChildItem -LiteralPath Pipeline/assets/start_location -Filter '*.asset.json'){
   $path=$manifest.FullName
   Invoke-Step ("fbx-"+$manifest.BaseName) {& $BlenderExe --background --python-exit-code 1 --python Pipeline/blender/build_asset.py -- --manifest $path *> (Join-Path $out ($manifest.BaseName+'.log'))}
  }
  Invoke-Step 'source-city-details' {& $PythonExe Scripts/assets/build_city_detail_plan.py}
  Invoke-Step 'source-start-placement' {& $PythonExe Scripts/assets/build_start_location_plan.py}
 }
 if($Mode -eq 'Full'){
  if(!(Test-Path -LiteralPath $editor)){throw 'Unreal executable missing; provide EngineRoot.'}
  Invoke-Step 'native-module' {& (Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat') WroclawTheGameEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE *> (Join-Path $out 'module.log')}
  if(!(Test-Path -LiteralPath Saved/FreeModelImportMap.json)){
   Invoke-Step 'existing-library-import' {Invoke-Editor 'library' 'Scripts/import_free_models.py' 'Saved/FreeModelsReady.ok'}
  }
  Invoke-Step 'shared-materials' {Invoke-Editor 'materials' 'Scripts/assets/prepare_start_location_materials.py' 'Saved/StartLocation/materials.ok'}
  Invoke-Step 'foliage-materials' {Invoke-Editor 'foliage' 'Scripts/assets/prepare_foliage_materials.py' 'Saved/StartLocation/foliage.ok'}
  $env:WTG_ASSET_MANIFESTS=ConvertTo-Json -Compress -InputObject @((Get-ChildItem -LiteralPath Pipeline/assets/start_location -Filter '*.asset.json').FullName)
  Invoke-Step 'native-model-import' {Invoke-Editor 'import' 'Pipeline/unreal/import_batch.py' 'Saved/Pipeline/import/batch.ok'}
  Invoke-Step 'native-asset-verification' {Invoke-Editor 'assets' 'Scripts/assets/verify_start_location_assets.py' 'Saved/StartLocation/assets.ok'}
  Invoke-Step 'native-base-map' {Invoke-Editor 'base' 'Scripts/prepare_playable_district.py' 'Saved/PlayableDistrictReady.ok'}
  Invoke-Step 'native-presentation' {Invoke-Editor 'presentation' 'Scripts/prepare_district_presentation.py' 'Saved/DistrictPresentationReady.ok'}
  Invoke-Step 'native-city-details' {Invoke-Editor 'details' 'Scripts/assets/place_city_details.py' 'Saved/CityDetails/placement.ok'}
  Invoke-Step 'native-start-placement' {Invoke-Editor 'placement' 'Scripts/assets/place_start_location.py' 'Saved/StartLocation/placement.ok'}
  Invoke-Step 'native-quality' {Invoke-Editor 'quality' 'Scripts/assets/prepare_start_location_quality.py' 'Saved/StartLocation/quality.ok'}
  $env:WTG_ASSET_MANIFEST='Data/start_location_qa.json'
  Invoke-Step 'rendered-capture' {Invoke-Editor 'capture' 'Pipeline/unreal/capture_qa.py' 'Saved/Pipeline/screenshots/start-location/capture.ok' -Render}
  if($GameplayQA){
   Invoke-Step 'district-regressions' {& powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/Test-PlayableDistrict.ps1 -EngineRoot $EngineRoot}
   Invoke-Step 'actual-start-gameplay' {& powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/Test-StartLocation.ps1 -EngineRoot $EngineRoot}
  }
 }
 Invoke-Step 'asset-catalog' {& $PythonExe Scripts/assets/write_start_location_catalog.py}
 Save-State 'PASS';Write-Output "PASS selected mode: $Mode"
}catch{if($steps.Count){$steps[$steps.Count-1].status='FAIL'};Save-State 'FAIL';throw}finally{
 $env:WTG_ASSET_MANIFESTS=$previousBatch;$env:WTG_ASSET_MANIFEST=$previousManifest;Pop-Location
}
