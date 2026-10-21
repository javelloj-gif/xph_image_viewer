$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot

function Resolve-Tool {
    param([string]$Name, [string]$ExplicitPath, [string[]]$Candidates = @())
    if ($ExplicitPath) {
        if (!(Test-Path -LiteralPath $ExplicitPath -PathType Leaf)) { throw "Tool not found: $ExplicitPath" }
        return (Resolve-Path -LiteralPath $ExplicitPath).Path
    }
    $command = Get-Command $Name -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($command) { return $command.Source }
    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate -PathType Leaf)) { return $candidate }
    }
    throw "Missing $Name. Install it, add it to PATH, or supply its explicit path."
}

function Get-VisualStudioRoot {
    $vswhere = Resolve-Tool 'vswhere.exe' '' @("${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe")
    $installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($LASTEXITCODE -ne 0 -or !$installation) { throw 'Visual Studio C++ x64 tools were not found.' }
    return ($installation | Select-Object -First 1)
}

function Get-CMake {
    param([string]$ExplicitPath)
    $command = Get-Command cmake.exe -ErrorAction SilentlyContinue
    if ($ExplicitPath -or $command) { return (Resolve-Tool 'cmake.exe' $ExplicitPath) }
    $vsRoot = Get-VisualStudioRoot
    return (Resolve-Tool 'cmake.exe' '' @((Join-Path $vsRoot 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe')))
}

function Get-ProjectVersion {
    $contents = Get-Content -LiteralPath (Join-Path $ProjectRoot 'CMakeLists.txt') -Raw
    $match = [regex]::Match($contents, '(?s)project\(\s*lenteuv\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)')
    if (!$match.Success) { throw 'Cannot read project version from CMakeLists.txt.' }
    return $match.Groups[1].Value
}

function Assert-NativeSuccess {
    param([string]$Action)
    if ($LASTEXITCODE -ne 0) { throw "$Action failed with exit code $LASTEXITCODE" }
}

function Assert-ChildPath {
    param([string]$Path, [string]$Parent)
    $resolvedPath = [IO.Path]::GetFullPath($Path)
    $resolvedParent = [IO.Path]::GetFullPath($Parent).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    if (!$resolvedPath.StartsWith($resolvedParent, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Path escapes expected directory: $resolvedPath"
    }
    return $resolvedPath
}
