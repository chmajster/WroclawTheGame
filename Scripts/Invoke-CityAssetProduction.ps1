[CmdletBinding()]
param(
    [string]$EngineRoot='D:\SteamLibrary\UnrealEngine\UE_5.8',
    [string]$Blender='C:\Program Files\Blender Foundation\Blender 5.2\blender.exe',
    [string]$Python='python',
    [switch]$SourcesOnly,
    [switch]$RebuildSources,
    [switch]$DownloadReviewed,
    [switch]$PrepareDistrict,
    [switch]$GameplayChecks
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$Root=Split-Path $PSScriptRoot -Parent
$RunId=[DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss')
$Output=Join-Path $Root 'Saved/CityDetails'
New-Item -ItemType Directory -Path $Output -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $Root 'Saved/Logs') -Force | Out-Null
$Report=Join-Path $Output 'production.json'
$Marker=Join-Path $Output 'production.ok'
Remove-Item -LiteralPath $Marker -ErrorAction SilentlyContinue
$State=@{status='IN_PROGRESS';run_id=$RunId;stages=@();native='NOT_RUN';visual_review='NOT_REVIEWED';next='source audit'}
$OldPythonPath=$env:PYTHONPATH
$OldManifest=$env:WTG_ASSET_MANIFEST
$OldManifests=$env:WTG_ASSET_MANIFESTS
function Save-State { [IO.File]::WriteAllText($Report,($State | ConvertTo-Json -Depth 12),[Text.UTF8Encoding]::new($false)) }
function Complete-Stage([string]$Name,[string]$Next) {
    $State.stages+=@{name=$Name;status='PASS';utc=[DateTime]::UtcNow.ToString('o')}
    $State.next=$Next; Save-State; Write-Output "PASS $Name"
}
function Run-Python([string[]]$Arguments) {
    & $Python @Arguments
    if($LASTEXITCODE -ne 0){throw "Python failed: $($Arguments -join ' ')"}
}
function Run-Editor([string]$Script,[string]$Name,[string]$Completion,[switch]$Render) {
    $Completion=Join-Path $Root $Completion
    Remove-Item -LiteralPath $Completion -ErrorAction SilentlyContinue
    $NativeArgs=@($Project,'/Engine/Maps/Entry',"-ExecutePythonScript=$Root/$Script",'-unattended','-nosplash',"-abslog=$Root/Saved/Logs/$Name.log")
    if(!$Render){$NativeArgs+='-nullrhi'}
    $Started=[DateTime]::UtcNow
    & $Editor @NativeArgs *> "$Root/Saved/Logs/$Name-stdout.log"
    $Code=$LASTEXITCODE
    $Log=Get-Content -LiteralPath "$Root/Saved/Logs/$Name.log" -Raw
    if($Code -ne 0 -or !(Test-Path -LiteralPath $Completion)){throw "Native stage failed: $Name; inspect its log"}
    if((Get-Item -LiteralPath $Completion).LastWriteTimeUtc -lt $Started){throw "Stale native marker: $Name"}
    if($Log -match 'LogPython: Error:|LogEditorPythonExecuter: Error:|Fatal error:|Failed to compile Material'){throw "Native stage errors: $Name"}
}
Push-Location $Root
try {
    Save-State
    $env:PYTHONPATH="$Root/Scripts/gis;$Root/Scripts;$Root/Pipeline/qa"
    if($OldPythonPath){$env:PYTHONPATH+=';'+$OldPythonPath}
    Run-Python @('Scripts/assets/audit_city_assets.py')
    $Acquire=@('Scripts/assets/acquire_city_assets.py')
    if($DownloadReviewed){$Acquire+='--download'}
    Run-Python $Acquire
    Run-Python @('Pipeline/qa/validate_manifest.py','--all')
    Run-Python @('-m','unittest','discover','-s','Tests','-p','test_city_asset*.py')
    Run-Python @('-m','unittest','discover','-s','Tests','-p','test_city_detail_plan.py')
    Complete-Stage 'sources/licenses/manifests/tests' 'author/export'
    $Manifests=@(Get-ChildItem -LiteralPath "$Root/Pipeline/assets/tenements" -Filter '*.asset.json' | Sort-Object Name)
    if($Manifests.Count -ne 5){throw 'Expected five tenement production manifests'}
    if($RebuildSources){
        if(!(Test-Path -LiteralPath $Blender)){throw 'Blender missing; specify -Blender'}
        Run-Python @('Scripts/assets/make_kit_textures.py')
        & $Blender --background --python-exit-code 1 --python "$Root/Scripts/assets/build_tenement_kit.py" -- --render *> "$Root/Saved/Logs/CitySourceAuthoring.log"
        if($LASTEXITCODE -ne 0){throw 'Blender source generation failed'}
        Complete-Stage 'Blender authoring' 'export'
    }
    if(!(Test-Path -LiteralPath "$Root/Saved/PlayableDistrict/facades.json") -or $PrepareDistrict){
        Run-Python @('Scripts/gis/build_playable_district.py')
    }
    Run-Python @('Scripts/assets/build_city_detail_plan.py')
    Run-Python @('Scripts/assets/check_city_asset_budget.py')
    Complete-Stage 'GIS plan/budget' 'FBX/native import'
    if($SourcesOnly){
        $State.status='SOURCE_CHECKS_PASS';$State.next='Run without -SourcesOnly for Blender FBX and native import/placement/QA'
        Save-State; Write-Output 'PASS source checks; native and visual QA NOT_RUN'; return
    }
    if(!(Test-Path -LiteralPath $Blender)){throw 'Blender missing; specify -Blender or use -SourcesOnly'}
    $Project=Join-Path $Root 'WroclawTheGame.uproject'
    $Editor=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
    if(!(Test-Path -LiteralPath $Editor)){throw 'Unreal Editor missing; specify -EngineRoot or use -SourcesOnly'}
    & "$Root/Scripts/Assert-UnrealVersion.ps1" -EngineRoot $EngineRoot -Project $Project -ExpectedVersion '5.8'
    foreach($Manifest in $Manifests){
        & $Blender --background --python-exit-code 1 --python "$Root/Pipeline/blender/build_asset.py" -- --manifest $Manifest.FullName *> "$Root/Saved/Logs/$($Manifest.BaseName)-Blender.log"
        if($LASTEXITCODE -ne 0){throw "Blender export failed: $($Manifest.Name)"}
    }
    Complete-Stage 'five Blender FBX products' 'editor module build'
    & (Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat') WroclawTheGameEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReloadFromIDE *> "$Root/Saved/Logs/CityProductionBuild.log"
    if($LASTEXITCODE -ne 0){throw 'Editor module build failed; inspect CityProductionBuild.log'}
    if($PrepareDistrict){
        # Explicit reconstruction uses the existing district workflow. Its free
        # catalog dependency stays explicit; never invent a successful import.
        & "$Root/Scripts/Build-PlayableDistrict.ps1" -EngineRoot $EngineRoot -Blender $Blender -Python $Python
        if(!$?){throw 'District preparation failed'}
    }
    if(!(Test-Path -LiteralPath "$Root/Content/Maps/Nadodrze_Playable.umap")){throw 'District map missing: run with -PrepareDistrict (requires imported free catalog)'}
    $env:WTG_ASSET_MANIFESTS=ConvertTo-Json -InputObject @($Manifests | ForEach-Object {$_.FullName}) -Compress
    Run-Editor 'Pipeline/unreal/import_batch.py' 'CityProductionImport' 'Saved/Pipeline/import/batch.ok'
    Complete-Stage 'five native imports' 'GIS placement'
    Run-Editor 'Scripts/assets/place_city_details.py' 'CityProductionPlacement' 'Saved/CityDetails/placement.ok'
    Run-Editor 'Scripts/assets/configure_city_asset_quality.py' 'CityProductionQuality' 'Saved/CityDetails/quality.ok'
    Run-Editor 'Scripts/assets/prepare_city_asset_qa.py' 'CityProductionQASetup' 'Saved/CityDetails/qa-map.ok'
    Complete-Stage 'native placement/quality/studio' 'asset Automation'
    $Automation="$Root/Saved/Pipeline/automation/city-production-$RunId"
    & $Editor $Project /Engine/Maps/Entry -nullrhi -unattended -nosplash '-ExecCmds=Automation RunTests WTG.Assets.TenementKit' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$Automation" "-abslog=$Root/Saved/Logs/CityProductionAutomation.log" *> "$Root/Saved/Logs/CityProductionAutomation-stdout.log"
    if($LASTEXITCODE -ne 0){throw 'Native asset test process failed'}
    Run-Python @('Pipeline/qa/check_automation.py',$Automation,'WTG.Assets.TenementKit')
    $State.automation_report=$Automation
    Complete-Stage 'native five-asset Automation' 'rendered QA views'
    $Views=(Get-Content -LiteralPath 'Data/city_asset_qa_views.json' -Raw | ConvertFrom-Json).views
    foreach($View in $Views){
        $ViewFile="Saved/CityDetails/$($View.id).views.json"
        [IO.File]::WriteAllText((Join-Path $Root $ViewFile),($View | ConvertTo-Json -Depth 6),[Text.UTF8Encoding]::new($false))
        $env:WTG_ASSET_MANIFEST=$ViewFile
        Run-Editor 'Pipeline/unreal/capture_qa.py' "CityProduction-$($View.id)" "Saved/Pipeline/screenshots/$($View.id)/capture.ok" -Render
    }
    Complete-Stage 'five fresh rendered views' 'optional gameplay checks/visual review'
    if($GameplayChecks){
        & "$Root/Scripts/Test-PlayableDistrict.ps1" -EngineRoot $EngineRoot
        if(!$?){throw 'District gameplay regression checks failed'}
        Complete-Stage 'district rules/save/session/replay' 'visual review'
    }
    $State.native='PASS';$State.status='EXECUTION_PASS';$State.next='Inspect five PNG views; native success does not establish visual acceptance or whole-city performance'
    $State.inputs=@{}
    foreach($File in @($Manifests.FullName)+@('Data/city_detail_policy.json','Data/city_asset_quality.json','Data/city_asset_acquisition.json','Data/city_asset_qa_views.json')){
        $State.inputs[$File]=(Get-FileHash -LiteralPath $File -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    Save-State; 'PASS' | Set-Content -LiteralPath $Marker -Encoding UTF8
    Write-Output 'PASS production execution; inspect Saved/CityDetails/production.json and five fresh QA PNGs'
} catch {
    $State.status='FAILED';$State.error=$_.Exception.Message; Save-State; throw
} finally {
    $env:PYTHONPATH=$OldPythonPath;$env:WTG_ASSET_MANIFEST=$OldManifest;$env:WTG_ASSET_MANIFESTS=$OldManifests
    Pop-Location
}
