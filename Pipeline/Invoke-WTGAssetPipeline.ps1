[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Manifest,
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [string]$BlenderExe = 'blender',
    [string]$PythonExe = 'python',
    [ValidateSet('validate','blender','unreal_import','automation','screenshots','visual_review','report')]
    [string]$From = 'validate',
    [switch]$Resume
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$ProjectRoot = Split-Path $PSScriptRoot -Parent
$Project = Join-Path $ProjectRoot 'WroclawTheGame.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if ([IO.Path]::IsPathRooted($Manifest)) {
    $ManifestPath = [IO.Path]::GetFullPath($Manifest)
} else {
    $ManifestPath = [IO.Path]::GetFullPath((Join-Path $ProjectRoot $Manifest))
}
foreach ($Required in @($Project, $Editor, $ManifestPath)) {
    if (-not (Test-Path -LiteralPath $Required)) { throw "Missing required file: $Required" }
}

$ManifestData = Get-Content -LiteralPath $ManifestPath -Raw | ConvertFrom-Json
$AssetId = [string]$ManifestData.id
if (-not $AssetId) { throw 'Manifest id is empty' }

$StateRoot = Join-Path $ProjectRoot 'Saved\Pipeline\status'
$StateFile = Join-Path $StateRoot "$AssetId.json"
New-Item -ItemType Directory -Force -Path $StateRoot | Out-Null

$Stages = @('validate','blender','unreal_import','automation','screenshots','visual_review','report')

function Write-State([string]$Stage, [string]$Status, [string]$Message = '') {
    $Record = [ordered]@{
        asset_id = $AssetId
        manifest = $ManifestPath
        stage = $Stage
        status = $Status
        message = $Message
        updated_at = [DateTimeOffset]::UtcNow.ToString('o')
    }
    $Record | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $StateFile -Encoding utf8
}

function Assert-Exit([string]$Label) {
    if ($LASTEXITCODE -ne 0) { throw "$Label failed with exit code $LASTEXITCODE" }
}

$StartStage = $From
if ($Resume -and (Test-Path -LiteralPath $StateFile)) {
    $Previous = Get-Content -LiteralPath $StateFile -Raw | ConvertFrom-Json
    if ($Previous.stage -eq 'done' -and $Previous.status -eq 'PASS') {
        Write-Host "Pipeline already complete for $AssetId"
        return
    }
    $PreviousIndex = [Array]::IndexOf($Stages, [string]$Previous.stage)
    if ($PreviousIndex -ge 0) {
        if ($Previous.status -eq 'PASS') {
            $NextIndex = $PreviousIndex + 1
            if ($NextIndex -lt $Stages.Count) { $StartStage = $Stages[$NextIndex] }
        } else {
            $StartStage = [string]$Previous.stage
        }
    }
}

$StartIndex = [Array]::IndexOf($Stages, $StartStage)
if ($StartIndex -lt 0) { throw "Unknown start stage: $StartStage" }

$env:WTG_ASSET_MANIFEST = $ManifestPath

for ($Index = $StartIndex; $Index -lt $Stages.Count; $Index++) {
    $Stage = $Stages[$Index]
    Write-State $Stage 'RUNNING'
    Write-Host "[$AssetId] $Stage"

    try {
        switch ($Stage) {
            'validate' {
                & $PythonExe (Join-Path $ProjectRoot 'Pipeline\qa\validate_manifest.py') $ManifestPath
                Assert-Exit 'Manifest validation'
            }
            'blender' {
                Remove-Item -LiteralPath (Join-Path $ProjectRoot "Saved\Pipeline\visual\$AssetId.json") -Force -ErrorAction SilentlyContinue
                & $BlenderExe --background --python (Join-Path $ProjectRoot 'Pipeline\blender\build_asset.py') -- --manifest $ManifestPath
                Assert-Exit 'Blender asset build'
                $Report = Join-Path $ProjectRoot "Saved\Pipeline\generated\$AssetId\blender-report.json"
                if (-not (Test-Path -LiteralPath $Report)) { throw "Blender report missing: $Report" }
            }
            'unreal_import' {
                $Marker = Join-Path $ProjectRoot "Saved\Pipeline\import\$AssetId.ok"
                Remove-Item -LiteralPath $Marker -Force -ErrorAction SilentlyContinue
                $Script = Join-Path $ProjectRoot 'Pipeline\unreal\import_asset.py'
                & $Editor $Project "-ExecutePythonScript=$Script" -unattended -nosplash -nullrhi -nop4 -stdout -FullStdOutLogOutput
                Assert-Exit 'Unreal asset import process'
                if (-not (Test-Path -LiteralPath $Marker)) { throw "Unreal import marker missing: $Marker" }
            }
            'automation' {
                $AutomationRoot = Join-Path $ProjectRoot "Saved\Pipeline\automation\$AssetId"
                Remove-Item -LiteralPath $AutomationRoot -Recurse -Force -ErrorAction SilentlyContinue
                New-Item -ItemType Directory -Force -Path $AutomationRoot | Out-Null
                $Filter = [string]$ManifestData.qa.automation_filter
                & $Editor $Project ([string]$ManifestData.unreal.qa_map) "-ExecCmds=Automation RunTests $Filter" '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$AutomationRoot" -unattended -nosplash -nullrhi -nop4 -stdout -FullStdOutLogOutput
                $AutomationExit = $LASTEXITCODE
                $AutomationReportRoot = Join-Path $ProjectRoot 'Saved\Pipeline\automation'
                New-Item -ItemType Directory -Force -Path $AutomationReportRoot | Out-Null
                [ordered]@{
                    asset_id = $AssetId
                    status = $(if ($AutomationExit -eq 0) { 'PASS' } else { 'FAIL' })
                    exit_code = $AutomationExit
                    filter = $Filter
                    report_directory = $AutomationRoot
                } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $AutomationReportRoot "$AssetId.json") -Encoding utf8
                if ($AutomationExit -ne 0) { throw "Unreal Automation failed with exit code $AutomationExit" }
            }
            'screenshots' {
                $CaptureRoot = Join-Path $ProjectRoot "Saved\Pipeline\screenshots\$AssetId"
                Remove-Item -LiteralPath (Join-Path $CaptureRoot 'capture.ok') -Force -ErrorAction SilentlyContinue
                $Script = Join-Path $ProjectRoot 'Pipeline\unreal\capture_qa.py'
                & $Editor $Project "-ExecutePythonScript=$Script" -unattended -nosplash -nop4 -stdout -FullStdOutLogOutput
                Assert-Exit 'Unreal screenshot process'
                if (-not (Test-Path -LiteralPath (Join-Path $CaptureRoot 'capture.ok'))) {
                    throw "Screenshot QA marker missing in $CaptureRoot"
                }
            }
            'visual_review' {
                $Cameras = @($ManifestData.qa.cameras)
                if ($Cameras.Count -gt 0) {
                    $Visual = Join-Path $ProjectRoot "Saved\Pipeline\visual\$AssetId.json"
                    if (-not (Test-Path -LiteralPath $Visual)) {
                        Write-State $Stage 'WAITING_FOR_VISUAL_REVIEW' "Inspect Saved/Pipeline/screenshots/$AssetId and record PASS/FAIL"
                        Write-Host "Visual review required. Inspect Saved/Pipeline/screenshots/$AssetId"
                        Write-Host ('Record result with: python Pipeline/qa/record_visual_review.py "{0}" --status PASS --notes "<comparison notes>"' -f $ManifestPath)
                        exit 3
                    }
                    $Review = Get-Content -LiteralPath $Visual -Raw | ConvertFrom-Json
                    if ($Review.status -ne 'PASS') { throw "Visual review is $($Review.status): $($Review.notes)" }
                }
            }
            'report' {
                & $PythonExe (Join-Path $ProjectRoot 'Pipeline\qa\build_report.py') $ManifestPath
                Assert-Exit 'Final QA report'
                $Final = Get-Content -LiteralPath (Join-Path $ProjectRoot "Saved\Pipeline\reports\$AssetId.json") -Raw | ConvertFrom-Json
                if ($Final.status -ne 'PASS') { throw "Final report status is $($Final.status)" }
            }
        }
        Write-State $Stage 'PASS'
    }
    catch {
        Write-State $Stage 'FAIL' $_.Exception.Message
        throw
    }
}

Write-State 'done' 'PASS' 'All deterministic gates and required visual review passed'
Write-Host "Pipeline PASS: $AssetId"
