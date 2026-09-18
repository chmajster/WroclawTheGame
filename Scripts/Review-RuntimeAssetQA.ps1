[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [Parameter(Mandatory=$true)][ValidateSet('PASS','FAIL')][string]$Status,
    [Parameter(Mandatory=$true)][string]$Notes,
    [string]$Reviewer = $env:USERNAME
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$Root=Split-Path $PSScriptRoot -Parent
$Python=Join-Path $EngineRoot 'Engine\Binaries\ThirdParty\Python3\Win64\python.exe'
if(-not(Test-Path -LiteralPath $Python)){throw "Missing Unreal Python: $Python"}
$Script=Join-Path $PSScriptRoot 'record_runtime_asset_visual_review.py'
& $Python $Script --status $Status --notes $Notes --reviewer $Reviewer
if($LASTEXITCODE -ne 0){throw "Runtime asset visual review recorded as $Status or validation failed"}
$Report=Join-Path $Root 'Saved\RuntimeAssetQA\visual_review.json'
if(-not(Test-Path -LiteralPath $Report)){throw 'Visual review report was not created'}
Write-Host "Runtime asset visual review: $Status"
Write-Host $Report
