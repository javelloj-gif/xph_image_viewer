# Release pipeline, retaining the original entry point and skip switches.
param(
    [switch]$SkipBuild,
    [switch]$SkipIcons,
    [string]$Version,
    [ValidateSet('Portable', 'Installer', 'All')][string]$Format = 'All',
    [string]$CMakePath,
    [string]$ISCCPath,
    [string]$DumpBinPath,
    [string]$RuntimeDirectory,
    [string]$PythonPath = 'python'
)
. (Join-Path $PSScriptRoot 'common.ps1')
if (!$Version) { $Version = Get-ProjectVersion }
if (!$SkipIcons) {
    & $PythonPath (Join-Path $PSScriptRoot 'generate_icons.py')
    Assert-NativeSuccess 'Icon generation'
}
if (!$SkipBuild) {
    & (Join-Path $PSScriptRoot 'build.ps1') -Configuration Release -CMakePath $CMakePath -Test
}
& (Join-Path $PSScriptRoot 'create_portable_zip.ps1') -SkipBuild -Version $Version -Format $Format -CMakePath $CMakePath -ISCCPath $ISCCPath -DumpBinPath $DumpBinPath -RuntimeDirectory $RuntimeDirectory
