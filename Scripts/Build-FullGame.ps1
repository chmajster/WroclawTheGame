[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [string]$Python='python',
    [switch]$PrepareOnly
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest

$Root=Split-Path $PSScriptRoot -Parent
$Args=@{
    EngineRoot=$EngineRoot
    Python=$Python
    City=$true
    Campaign=$true
}
if(-not $PrepareOnly){$Args.Package=$true}

& (Join-Path $PSScriptRoot 'Build-Geography.ps1') @Args
if(-not $?){throw 'Full-game GIS build pipeline failed'}

if($PrepareOnly){
    Write-Host 'Full-game GIS content prepared. Runtime acceptance still requires UE playtest.'
    return
}

$Archive=Join-Path $Root 'Builds\FullGame'
$Executables=@(Get-ChildItem -LiteralPath $Archive -Filter 'WroclawTheGame.exe' -Recurse -ErrorAction SilentlyContinue)
if($Executables.Count -eq 0){throw 'Full-game packaging returned without WroclawTheGame.exe'}
Write-Host "Full-game package: $Archive"
Write-Host 'The package starts on /Game/Maps/Nadodrze_GIS. Check docs/ACCEPTANCE.md before promoting any sector to Playable.'
