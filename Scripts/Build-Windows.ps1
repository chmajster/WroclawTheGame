[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [ValidateSet('Development','Shipping')][string]$Configuration = 'Development',
    [string]$OutputDirectory,
    [switch]$PrepareOnly,
    [switch]$BuildHLOD,
    [switch]$InstallPrerequisites
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest


function Test-ExclusiveFileAccess {
    param([Parameter(Mandatory=$true)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        return $true
    }

    try {
        $Stream = [System.IO.File]::Open(
            $Path,
            [System.IO.FileMode]::Open,
            [System.IO.FileAccess]::ReadWrite,
            [System.IO.FileShare]::None
        )
        $Stream.Dispose()
        return $true
    }
    catch [System.IO.IOException] {
        return $false
    }
}

function Stop-UnrealEditorProjectDllLock {
    param(
        [Parameter(Mandatory=$true)][string]$EngineRoot,
        [Parameter(Mandatory=$true)][string]$ProjectRoot
    )

    $ProjectEditorDll = Join-Path $ProjectRoot 'Binaries\Win64\UnrealEditor-WroclawTheGame.dll'
    if (Test-ExclusiveFileAccess -Path $ProjectEditorDll) {
        return
    }

    $EngineBin = [System.IO.Path]::GetFullPath(
        (Join-Path $EngineRoot 'Engine\Binaries\Win64')
    ).TrimEnd('\') + '\'

    $EditorProcesses = @(
        Get-Process -Name 'UnrealEditor','UnrealEditor-Cmd' -ErrorAction SilentlyContinue |
            Where-Object {
                try {
                    $_.Path -and
                    [System.IO.Path]::GetFullPath($_.Path).StartsWith(
                        $EngineBin,
                        [System.StringComparison]::OrdinalIgnoreCase
                    )
                }
                catch {
                    $false
                }
            }
    )

    if ($EditorProcesses.Count -eq 0) {
        throw "Project editor DLL is locked and no Unreal Editor process from '$EngineRoot' could be identified. Close the process locking '$ProjectEditorDll' and retry."
    }

    foreach ($Process in $EditorProcesses) {
        Write-Host "Closing Unreal Editor process $($Process.Id) before linking project DLL..."

        $RequestedGracefulExit = $false
        try {
            if ($Process.MainWindowHandle -ne 0) {
                $RequestedGracefulExit = $Process.CloseMainWindow()
            }
        }
        catch {
            $RequestedGracefulExit = $false
        }

        if ($RequestedGracefulExit) {
            try {
                $Process.WaitForExit(5000)
            }
            catch {
                # Fall back to a forced stop below.
            }
        }

        if (-not $Process.HasExited) {
            Stop-Process -Id $Process.Id -Force -ErrorAction Stop
            try {
                $Process.WaitForExit(5000)
            }
            catch {
                # The lock check below is the final authority.
            }
        }
    }

    Start-Sleep -Milliseconds 500
    if (-not (Test-ExclusiveFileAccess -Path $ProjectEditorDll)) {
        throw "Unreal Editor was stopped, but '$ProjectEditorDll' is still locked. Close the remaining process holding the DLL and retry."
    }

    Write-Host 'Released project editor DLL lock.'
}

function Test-NetFxSdk {
    $SdkDirectories = @(
        (Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\NETFXSDK'),
        (Join-Path $env:ProgramFiles 'Windows Kits\NETFXSDK')
    )
    foreach ($Directory in $SdkDirectories) {
        if ((Test-Path -LiteralPath $Directory) -and
            @(Get-ChildItem -LiteralPath $Directory -Directory -ErrorAction SilentlyContinue).Count -gt 0) {
            return $true
        }
    }

    $RegistryRoots = @(
        'HKLM:\SOFTWARE\Microsoft\Microsoft SDKs\NETFXSDK',
        'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Microsoft SDKs\NETFXSDK'
    )
    foreach ($RegistryRoot in $RegistryRoots) {
        if ((Test-Path -LiteralPath $RegistryRoot) -and
            @(Get-ChildItem -LiteralPath $RegistryRoot -ErrorAction SilentlyContinue).Count -gt 0) {
            return $true
        }
    }
    return $false
}

if (-not (Test-NetFxSdk)) {
    if (-not $InstallPrerequisites) {
        throw 'Missing .NET Framework SDK required by UnrealBuildTool. Re-run with -InstallPrerequisites to install the Microsoft Developer Pack through winget.'
    }

    $Winget = Get-Command 'winget.exe' -ErrorAction SilentlyContinue
    if (-not $Winget) {
        throw 'winget.exe is required by -InstallPrerequisites. Install or update Windows App Installer first.'
    }

    Write-Host 'Installing Microsoft .NET Framework Developer Pack...'
    & $Winget.Source install --id Microsoft.DotNet.Framework.DeveloperPack_4 --exact --source winget --silent --accept-package-agreements --accept-source-agreements --disable-interactivity
    if ($LASTEXITCODE -notin @(0, 3010)) {
        throw ".NET Framework Developer Pack installation failed ($LASTEXITCODE)"
    }
    if (-not (Test-NetFxSdk)) {
        throw '.NET Framework Developer Pack installation completed, but the SDK is not visible yet. Restart PowerShell and run the build again.'
    }
}

$ProjectRoot = Split-Path $PSScriptRoot -Parent
$Project = Join-Path $ProjectRoot 'WroclawTheGame.uproject'
& (Join-Path $PSScriptRoot 'Assert-UnrealVersion.ps1') -EngineRoot $EngineRoot -Project $Project -ExpectedVersion '5.8'
$BuildTool = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$UnrealPython = Join-Path $EngineRoot 'Engine\Binaries\ThirdParty\Python3\Win64\python.exe'
$Automation = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
foreach ($File in @($BuildTool, $Editor, $Automation, $UnrealPython)) {
    if (-not (Test-Path -LiteralPath $File)) { throw "Missing Unreal Engine 5.8 tool: $File" }
}
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $ProjectRoot "Builds\$Configuration" }
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
$PipelineValidator = Join-Path $ProjectRoot 'Pipeline\qa\validate_manifest.py'
& $UnrealPython $PipelineValidator --all
if ($LASTEXITCODE -ne 0) { throw 'Asset production manifest validation failed' }
& $UnrealPython (Join-Path $PSScriptRoot 'compile_chapter.py')
if ($LASTEXITCODE -ne 0) { throw 'Chapter data validation failed before compilation' }
& $UnrealPython (Join-Path $PSScriptRoot 'compile_city_gameplay.py')
if ($LASTEXITCODE -ne 0) { throw 'City gameplay validation failed' }
& $UnrealPython (Join-Path $PSScriptRoot 'compile_world.py')
if ($LASTEXITCODE -ne 0) { throw 'World profile validation failed' }
& $UnrealPython (Join-Path $PSScriptRoot 'compile_tags.py')
if ($LASTEXITCODE -ne 0) { throw 'Gameplay tag generation failed' }
Stop-UnrealEditorProjectDllLock -EngineRoot $EngineRoot -ProjectRoot $ProjectRoot
& $BuildTool WroclawTheGameEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw "Editor target build failed ($LASTEXITCODE)" }
$CreatorMarker = Join-Path $ProjectRoot 'Saved\CharacterCreatorReady.ok'
$SurfaceMarker = Join-Path $ProjectRoot 'Saved\SurfaceQualityReady.ok'
if (Test-Path -LiteralPath $SurfaceMarker) { Remove-Item -LiteralPath $SurfaceMarker }
$SurfaceScript = Join-Path $PSScriptRoot 'prepare_surface_quality.py'
& $Editor $Project "-ExecutePythonScript=$SurfaceScript" -unattended -nosplash -stdout -FullStdOutLogOutput
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $SurfaceMarker)) { throw 'Surface PBR validation failed; packaging stopped' }

# External meshes and animation libraries must exist before character assets are validated.
$FreeModelsMarker = Join-Path $ProjectRoot 'Saved\FreeModelsReady.ok'
if (Test-Path -LiteralPath $FreeModelsMarker) { Remove-Item -LiteralPath $FreeModelsMarker }
$FreeModelsScript = Join-Path $PSScriptRoot 'import_free_models.py'
& $Editor $Project "-ExecutePythonScript=$FreeModelsScript" -unattended -nosplash -nullrhi -stdout -FullStdOutLogOutput
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $FreeModelsMarker)) { throw 'Free model import failed; packaging stopped' }

$FreeAnimationsMarker = Join-Path $ProjectRoot 'Saved\FreeAnimationsReady.ok'
if (Test-Path -LiteralPath $FreeAnimationsMarker) { Remove-Item -LiteralPath $FreeAnimationsMarker }
$FreeAnimationsScript = Join-Path $PSScriptRoot 'import_free_animations.py'
& $Editor $Project "-ExecutePythonScript=$FreeAnimationsScript" -unattended -nosplash -nullrhi -stdout -FullStdOutLogOutput
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $FreeAnimationsMarker)) { throw 'Free animation import failed; packaging stopped' }

if (Test-Path -LiteralPath $CreatorMarker) { Remove-Item -LiteralPath $CreatorMarker }
$CreatorScript = Join-Path $PSScriptRoot 'prepare_character_creator.py'
& $Editor $Project "-ExecutePythonScript=$CreatorScript" -unattended -nosplash -nullrhi -stdout -FullStdOutLogOutput
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $CreatorMarker)) { throw 'Character Creator asset validation failed; packaging stopped' }
$Marker = Join-Path $ProjectRoot 'Saved\GeneratedContent.ok'
if (Test-Path -LiteralPath $Marker) { Remove-Item -LiteralPath $Marker }
$Script = Join-Path $PSScriptRoot 'prepare_content.py'
& $Editor $Project "-ExecutePythonScript=$Script" -unattended -nosplash -nullrhi -stdout -FullStdOutLogOutput
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $Marker)) {
    throw 'Content generation failed. Read Saved\Logs\WroclawTheGame.log; packaging was not started.'
}
# Convert the authored level in place; its package path remains stable for saved games.
& $Editor $Project -run=WorldPartitionConvertCommandlet /Game/Maps/Przebudzenie_Source -SCCProvider=None -AllowCommandletRendering -unattended -stdout -FullStdOutLogOutput
if ($LASTEXITCODE -ne 0) { throw 'World Partition conversion failed' }
$VerifyScript = Join-Path $PSScriptRoot 'verify_partition.py'
$PartitionMarker = Join-Path $ProjectRoot 'Saved\PartitionVerified.ok'
if (Test-Path -LiteralPath $PartitionMarker) { Remove-Item -LiteralPath $PartitionMarker }
& $Editor $Project "-ExecutePythonScript=$VerifyScript" -unattended -nosplash -nullrhi -stdout -FullStdOutLogOutput
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $PartitionMarker)) { throw 'Streaming validation failed; packaging stopped' }
if ($BuildHLOD) {
    & $Editor $Project /Game/Maps/Przebudzenie_Source -run=WorldPartitionBuilderCommandlet -Builder=WorldPartitionHLODsBuilder -SetupHLODs -BuildHLODs -AllowCommandletRendering -SCCProvider=None -unattended
    if ($LASTEXITCODE -ne 0) { throw 'HLOD generation failed' }
}
if ($PrepareOnly) { Write-Host 'Editor target and generated content prepared.'; return }
& $Automation BuildCookRun "-project=$Project" -noP4 -platform=Win64 "-clientconfig=$Configuration" -build -cook '-map=/Game/Maps/Przebudzenie_Source' -stage -pak -archive "-archivedirectory=$OutputDirectory" -prereqs -utf8output -unattended
if ($LASTEXITCODE -ne 0) { throw "Windows packaging failed ($LASTEXITCODE)" }
$Executables = @(Get-ChildItem -LiteralPath $OutputDirectory -Filter 'WroclawTheGame.exe' -Recurse)
if ($Executables.Count -eq 0) { throw 'Packaging returned success but no game executable was found.' }
Write-Host "Windows package: $OutputDirectory"
Write-Host 'Build complete. Definition of Done still requires the manual end-to-end and performance checks in docs/ACCEPTANCE.md.'
