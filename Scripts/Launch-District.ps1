param([string]$EngineRoot = 'D:\SteamLibrary\UnrealEngine\UE_5.8')
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path $PSScriptRoot -Parent
$Project = Join-Path $ProjectRoot 'WroclawTheGame.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$Map = Join-Path $ProjectRoot 'Content\Maps\Nadodrze_Playable.umap'
if (!(Test-Path -LiteralPath $Editor)) { throw 'Podaj -EngineRoot wskazujący instalację Unreal Engine 5.8.' }
if (!(Test-Path -LiteralPath $Map)) { throw 'Najpierw uruchom Scripts/Build-PlayableDistrict.ps1.' }
if (!(Test-Path -LiteralPath (Join-Path $ProjectRoot 'Binaries\Win64\UnrealEditor-WroclawTheGame.dll'))) { throw 'Najpierw skompiluj WroclawTheGameEditor (Build-PlayableDistrict.ps1).' }
Start-Process -FilePath $Editor -WorkingDirectory $ProjectRoot -ArgumentList @(
    ('"{0}"' -f $Project), '/Game/Maps/Nadodrze_Playable', '-game', '-windowed',
    '-ResX=1600', '-ResY=900', '-nosplash', '-ExecCmds="DisableAllScreenMessages"'
)
Write-Output 'Uruchomiono Nadodrze. E: interakcja; F5/F9: zapis/odczyt; Esc: pauza.'
