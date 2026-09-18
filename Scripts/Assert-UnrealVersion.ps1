[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [Parameter(Mandatory=$true)][string]$Project,
    [string]$ExpectedVersion = '5.8'
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest

if (-not (Test-Path -LiteralPath $Project)) {
    throw "Unreal project file not found: $Project"
}
if (-not (Test-Path -LiteralPath $EngineRoot)) {
    throw "Unreal Engine root not found: $EngineRoot"
}

$ProjectJson = Get-Content -LiteralPath $Project -Raw | ConvertFrom-Json
$Association = [string]$ProjectJson.EngineAssociation
if ($Association -ne $ExpectedVersion) {
    throw "Project EngineAssociation is '$Association'; expected '$ExpectedVersion'. Update WroclawTheGame.uproject intentionally before building with another engine."
}

$BuildVersionPath = Join-Path $EngineRoot 'Engine\Build\Build.version'
if (-not (Test-Path -LiteralPath $BuildVersionPath)) {
    throw "Cannot verify Unreal Engine version because Build.version is missing: $BuildVersionPath"
}
$BuildVersion = Get-Content -LiteralPath $BuildVersionPath -Raw | ConvertFrom-Json
$DetectedVersion = "$($BuildVersion.MajorVersion).$($BuildVersion.MinorVersion)"
if ($DetectedVersion -ne $ExpectedVersion) {
    throw "EngineRoot points to Unreal Engine $DetectedVersion; project requires $ExpectedVersion."
}

Write-Host "Verified Unreal Engine $ExpectedVersion: $EngineRoot"
