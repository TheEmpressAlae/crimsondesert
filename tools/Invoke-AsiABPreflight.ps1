[CmdletBinding()]
param(
    [Parameter()]
    [string] $GameRoot = 'C:\steammodded\steamapps\common\Crimson Desert',

    [Parameter()]
    [string[]] $DmmStagingRoot = @(),

    [Parameter()]
    [string] $OutputPath
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

function Get-NormalizedPath {
    param([Parameter(Mandatory = $true)][string] $Path)

    try {
        return [System.IO.Path]::GetFullPath($Path).TrimEnd('\')
    }
    catch {
        return $Path.TrimEnd('\')
    }
}

function Get-FileEvidence {
    param([Parameter(Mandatory = $true)][string] $Path)

    $normalized = Get-NormalizedPath -Path $Path
    if (-not (Test-Path -LiteralPath $normalized -PathType Leaf)) {
        return [pscustomobject][ordered]@{
            Path = $normalized
            Exists = $false
            Length = $null
            LastWriteTimeUtc = $null
            SHA256 = $null
            FileVersion = $null
            ProductVersion = $null
            Error = $null
        }
    }

    try {
        $item = Get-Item -LiteralPath $normalized
        $hash = Get-FileHash -LiteralPath $normalized -Algorithm SHA256
        return [pscustomobject][ordered]@{
            Path = $item.FullName
            Exists = $true
            Length = $item.Length
            LastWriteTimeUtc = $item.LastWriteTimeUtc.ToString('o')
            SHA256 = $hash.Hash
            FileVersion = $item.VersionInfo.FileVersion
            ProductVersion = $item.VersionInfo.ProductVersion
            Error = $null
        }
    }
    catch {
        return [pscustomobject][ordered]@{
            Path = $normalized
            Exists = $true
            Length = $null
            LastWriteTimeUtc = $null
            SHA256 = $null
            FileVersion = $null
            ProductVersion = $null
            Error = $_.Exception.Message
        }
    }
}

function Test-SamePath {
    param(
        [AllowNull()][string] $Left,
        [AllowNull()][string] $Right
    )

    if ([string]::IsNullOrWhiteSpace($Left) -or [string]::IsNullOrWhiteSpace($Right)) {
        return $false
    }

    return [string]::Equals(
        (Get-NormalizedPath -Path $Left),
        (Get-NormalizedPath -Path $Right),
        [System.StringComparison]::OrdinalIgnoreCase)
}

$resolvedGameRoot = Get-NormalizedPath -Path $GameRoot
$bin64 = Get-NormalizedPath -Path (Join-Path $resolvedGameRoot 'bin64')
$gameExe = Get-NormalizedPath -Path (Join-Path $bin64 'CrimsonDesert.exe')
$gameCrashpad = Get-NormalizedPath -Path (Join-Path $bin64 'crashpad_handler.exe')
$warnings = New-Object System.Collections.Generic.List[string]

$gameProcesses = @()
try {
    foreach ($process in @(Get-Process -Name 'CrimsonDesert' -ErrorAction SilentlyContinue)) {
        $processPath = $null
        $startTime = $null
        try { $processPath = $process.Path } catch { }
        try { $startTime = $process.StartTime.ToUniversalTime().ToString('o') } catch { }

        $moduleEvidence = @()
        $moduleEnumerationPermitted = $true
        $moduleError = $null
        try {
            $moduleEvidence = @($process.Modules | ForEach-Object {
                [pscustomobject][ordered]@{
                    ModuleName = $_.ModuleName
                    FileName = $_.FileName
                    BaseAddress = ('0x{0:X}' -f $_.BaseAddress.ToInt64())
                    ModuleMemorySize = $_.ModuleMemorySize
                    FileVersion = $_.FileVersionInfo.FileVersion
                }
            } | Sort-Object FileName, ModuleName)
        }
        catch {
            $moduleEnumerationPermitted = $false
            $moduleError = $_.Exception.Message
        }

        $gameProcesses += [pscustomobject][ordered]@{
            Id = $process.Id
            Path = $processPath
            IsExpectedExecutable = Test-SamePath -Left $processPath -Right $gameExe
            StartTimeUtc = $startTime
            ModuleEnumerationPermitted = $moduleEnumerationPermitted
            ModuleError = $moduleError
            Modules = $moduleEvidence
        }
    }
}
catch {
    $warnings.Add("Game process enumeration failed: $($_.Exception.Message)")
}

$crashpadProcesses = @()
try {
    foreach ($process in @(Get-Process -Name 'crashpad_handler' -ErrorAction SilentlyContinue)) {
        $processPath = $null
        $startTime = $null
        try { $processPath = $process.Path } catch { }
        try { $startTime = $process.StartTime.ToUniversalTime().ToString('o') } catch { }

        $classification = if ([string]::IsNullOrWhiteSpace($processPath)) {
            'UnknownPath'
        }
        elseif (Test-SamePath -Left $processPath -Right $gameCrashpad) {
            'CrimsonDesert'
        }
        else {
            'Other'
        }

        $crashpadProcesses += [pscustomobject][ordered]@{
            Id = $process.Id
            ExecutablePath = $processPath
            Classification = $classification
            StartTimeUtc = $startTime
        }
    }
}
catch {
    $warnings.Add("Crashpad process enumeration failed: $($_.Exception.Message)")
}

$activeFiles = New-Object System.Collections.Generic.List[string]
$activeFiles.Add($gameExe)
foreach ($loaderName in @('version.dll', 'winmm.dll', 'dinput8.dll', 'dsound.dll')) {
    $loaderPath = Join-Path $bin64 $loaderName
    if (Test-Path -LiteralPath $loaderPath -PathType Leaf) {
        $activeFiles.Add((Get-NormalizedPath -Path $loaderPath))
    }
}
if (Test-Path -LiteralPath $bin64 -PathType Container) {
    foreach ($item in @(Get-ChildItem -LiteralPath $bin64 -File | Where-Object {
        $_.Extension -ieq '.asi' -or $_.Extension -ieq '.ini'
    } | Sort-Object FullName)) {
        $activeFiles.Add($item.FullName)
    }
}
else {
    $warnings.Add("bin64 directory not found: $bin64")
}
$activeEvidence = @($activeFiles | Select-Object -Unique | ForEach-Object {
    Get-FileEvidence -Path $_
})

$publishRoot = Get-NormalizedPath -Path (Split-Path -Parent $PSScriptRoot)
$projectRoot = Get-NormalizedPath -Path (Split-Path -Parent (Split-Path -Parent $publishRoot))
$automaticStagingRoots = @(
    (Join-Path $publishRoot 'package-v1.1.0'),
    (Join-Path $projectRoot 'marker-teleport-asi\package\DMM-installed-layout'),
    (Join-Path $env:LOCALAPPDATA 'com.definitive.modmanager'),
    (Join-Path $env:APPDATA 'com.definitive.modmanager'),
    (Join-Path $env:APPDATA 'DefinitiveModManager')
)
$stagingRoots = @($DmmStagingRoot + $automaticStagingRoots | Where-Object {
    -not [string]::IsNullOrWhiteSpace($_)
} | ForEach-Object {
    Get-NormalizedPath -Path $_
} | Select-Object -Unique)

$candidateNames = @(
    'MarkerTeleport.asi',
    'MarkerTeleport.ini',
    'MarkerTeleportASI.asi',
    'MarkerTeleportASI.ini',
    'OpenStorageAnywhere.asi',
    'OpenStorageAnywhere.ini'
)
$stagedFiles = @()
foreach ($root in $stagingRoots) {
    if (-not (Test-Path -LiteralPath $root -PathType Container)) {
        continue
    }

    try {
        $stagedFiles += @(Get-ChildItem -LiteralPath $root -Recurse -File -ErrorAction Stop | Where-Object {
            $candidateNames -icontains $_.Name
        } | ForEach-Object {
            [pscustomobject]@{ Root = $root; Item = $_ }
        })
    }
    catch {
        $warnings.Add("Staging scan failed for '$root': $($_.Exception.Message)")
    }
}

$stagedPairs = @($stagedFiles | Group-Object { $_.Item.DirectoryName } | ForEach-Object {
    $group = @($_.Group)
    $directory = $_.Name
    [pscustomobject][ordered]@{
        Directory = $directory
        DiscoveredUnder = @($group.Root | Select-Object -Unique)
        MarkerTeleport = @($group | Where-Object {
            $_.Item.Name -ilike 'MarkerTeleport.*'
        } | ForEach-Object {
            Get-FileEvidence -Path $_.Item.FullName
        })
        MarkerTeleportASI = @($group | Where-Object {
            $_.Item.Name -ilike 'MarkerTeleportASI.*'
        } | ForEach-Object {
            Get-FileEvidence -Path $_.Item.FullName
        })
        OpenStorageAnywhere = @($group | Where-Object {
            $_.Item.Name -ilike 'OpenStorageAnywhere.*'
        } | ForEach-Object {
            Get-FileEvidence -Path $_.Item.FullName
        })
    }
} | Sort-Object Directory)

$result = [pscustomobject][ordered]@{
    SchemaVersion = 1
    CapturedAtUtc = [DateTime]::UtcNow.ToString('o')
    ReadOnlyPreflight = $true
    GameRoot = $resolvedGameRoot
    Bin64 = $bin64
    ExpectedGameExecutable = $gameExe
    ExpectedCrashpadExecutable = $gameCrashpad
    GameProcesses = $gameProcesses
    CrashpadProcesses = $crashpadProcesses
    ActiveFileEvidence = $activeEvidence
    DmmStagingRoots = $stagingRoots
    DmmStagedPairs = $stagedPairs
    Warnings = @($warnings)
}

$json = $result | ConvertTo-Json -Depth 8
if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $json
}
else {
    $resolvedOutputPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputPath)
    $outputDirectory = Split-Path -Parent $resolvedOutputPath
    if (-not [string]::IsNullOrWhiteSpace($outputDirectory) -and
        -not (Test-Path -LiteralPath $outputDirectory -PathType Container)) {
        throw "Output directory does not exist: $outputDirectory"
    }
    [System.IO.File]::WriteAllText($resolvedOutputPath, $json, [System.Text.UTF8Encoding]::new($false))
}
