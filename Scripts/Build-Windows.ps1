[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [ValidateSet('Development','Shipping')][string]$Configuration = 'Development',
    [string]$OutputDirectory,
    [switch]$PrepareOnly
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$ProjectRoot = Split-Path $PSScriptRoot -Parent
$Project = Join-Path $ProjectRoot 'WroclawTheGame.uproject'
$BuildTool = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$UnrealPython = Join-Path $EngineRoot 'Engine\Binaries\ThirdParty\Python3\Win64\python.exe'
$Automation = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
foreach ($File in @($BuildTool, $Editor, $Automation, $UnrealPython)) {
    if (-not (Test-Path -LiteralPath $File)) { throw "Missing Unreal Engine 5.6 tool: $File" }
}
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $ProjectRoot "Builds\$Configuration" }
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
& $UnrealPython (Join-Path $PSScriptRoot 'compile_chapter.py')
if ($LASTEXITCODE -ne 0) { throw 'Chapter data validation failed before compilation' }
& $BuildTool WroclawTheGameEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw "Editor target build failed ($LASTEXITCODE)" }
$Marker = Join-Path $ProjectRoot 'Saved\GeneratedContent.ok'
if (Test-Path -LiteralPath $Marker) { Remove-Item -LiteralPath $Marker }
$Script = Join-Path $PSScriptRoot 'prepare_content.py'
& $Editor $Project "-ExecutePythonScript=$Script" -unattended -nosplash -nullrhi -stdout -FullStdOutLogOutput
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $Marker)) {
    throw 'Content generation failed. Read Saved\Logs\WroclawTheGame.log; packaging was not started.'
}
if ($PrepareOnly) { Write-Host 'Editor target and generated content prepared.'; return }
& $Automation BuildCookRun "-project=$Project" -noP4 -platform=Win64 "-clientconfig=$Configuration" -build -cook '-map=/Game/Maps/Przebudzenie' -stage -pak -archive "-archivedirectory=$OutputDirectory" -prereqs -utf8output -unattended
if ($LASTEXITCODE -ne 0) { throw "Windows packaging failed ($LASTEXITCODE)" }
$Executables = @(Get-ChildItem -LiteralPath $OutputDirectory -Filter 'WroclawTheGame.exe' -Recurse)
if ($Executables.Count -eq 0) { throw 'Packaging returned success but no game executable was found.' }
Write-Host "Windows package: $OutputDirectory"
Write-Host 'Build complete. Definition of Done still requires the manual end-to-end and performance checks in docs/ACCEPTANCE.md.'
