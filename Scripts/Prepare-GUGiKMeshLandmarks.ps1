[CmdletBinding()]
param(
    [string]$Python='python',
    [string[]]$Target
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$Root=Split-Path $PSScriptRoot -Parent
$Script=Join-Path $PSScriptRoot 'gis\fetch_gugik_mesh_landmarks.py'
$Args=@('--config',(Join-Path $Root 'Data\gugik_mesh_landmarks.json'),'--output',(Join-Path $Root 'Saved\GUGiKMeshLandmarks'))
foreach($Id in $Target){$Args+=@('--target',$Id)}
& $Python $Script @Args
if($LASTEXITCODE -ne 0){throw 'GUGiK textured landmark preparation failed'}
Write-Host 'Candidates saved under Saved\GUGiKMeshLandmarks. Production publish still requires Blender/Unreal/visual QA gates.'
