[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Manifest,
    [switch]$Merge
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$ProjectRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
if ([IO.Path]::IsPathRooted($Manifest)) {
    $ManifestPath = [IO.Path]::GetFullPath($Manifest)
} else {
    $ManifestPath = [IO.Path]::GetFullPath((Join-Path $ProjectRoot $Manifest))
}
if (-not (Test-Path -LiteralPath $ManifestPath)) { throw "Manifest not found: $ManifestPath" }

$ManifestData = Get-Content -LiteralPath $ManifestPath -Raw | ConvertFrom-Json
$AssetId = [string]$ManifestData.id
$ReportPath = Join-Path $ProjectRoot "Saved\Pipeline\reports\$AssetId.json"
$ReportMd = Join-Path $ProjectRoot "Saved\Pipeline\reports\$AssetId.md"
$StatePath = Join-Path $ProjectRoot "Saved\Pipeline\status\$AssetId.json"
foreach ($Required in @($ReportPath, $ReportMd, $StatePath)) {
    if (-not (Test-Path -LiteralPath $Required)) { throw "Required QA artifact missing: $Required" }
}

$Report = Get-Content -LiteralPath $ReportPath -Raw | ConvertFrom-Json
$State = Get-Content -LiteralPath $StatePath -Raw | ConvertFrom-Json
if ($Report.status -ne 'PASS' -or $State.stage -ne 'done' -or $State.status -ne 'PASS') {
    throw "Publishing refused: pipeline is not PASS for $AssetId"
}

foreach ($Command in @('git','gh')) {
    if (-not (Get-Command $Command -ErrorAction SilentlyContinue)) { throw "Missing command: $Command" }
}

Push-Location $ProjectRoot
try {
    $CurrentBranch = (& git branch --show-current).Trim()
    if ($LASTEXITCODE -ne 0 -or -not $CurrentBranch) { throw 'Cannot determine current Git branch' }

    if ($CurrentBranch -eq 'main') {
        $BranchName = 'asset/' + ($AssetId -replace '[^A-Za-z0-9._/-]', '-')
        & git checkout -b $BranchName
        if ($LASTEXITCODE -ne 0) { throw "Cannot create branch $BranchName" }
        $CurrentBranch = $BranchName
    }

    & git add -A
    if ($LASTEXITCODE -ne 0) { throw 'git add failed' }
    & git diff --cached --quiet
    if ($LASTEXITCODE -eq 0) { throw 'No staged repository changes to publish' }
    if ($LASTEXITCODE -ne 1) { throw 'git diff --cached failed' }

    & git commit -m "feat(asset): $AssetId"
    if ($LASTEXITCODE -ne 0) { throw 'git commit failed' }

    & git push -u origin $CurrentBranch
    if ($LASTEXITCODE -ne 0) { throw 'git push failed' }

    $Body = Get-Content -LiteralPath $ReportMd -Raw
    $PrUrl = (& gh pr create --base main --head $CurrentBranch --title "feat(asset): $AssetId" --body $Body).Trim()
    if ($LASTEXITCODE -ne 0 -or -not $PrUrl) { throw 'gh pr create failed' }
    Write-Host "PR: $PrUrl"

    if ($Merge) {
        & gh pr merge $PrUrl --squash --delete-branch
        if ($LASTEXITCODE -ne 0) { throw 'gh pr merge failed' }
    }
}
finally {
    Pop-Location
}
