# Unified packaging from fresh staging. Never copies checkout user_data.
param(
    [string]$OutputName,
    [switch]$SkipBuild,
    [string]$Version,
    [ValidateSet('Portable', 'Installer', 'All')][string]$Format = 'Portable',
    [string]$CMakePath,
    [string]$ISCCPath,
    [string]$DumpBinPath,
    [string]$RuntimeDirectory
)
. (Join-Path $PSScriptRoot 'common.ps1')
if (!$Version) { $Version = Get-ProjectVersion }
if ($Version -notmatch '^\d+\.\d+\.\d+$') { throw 'Version must have major.minor.patch form.' }
if (!$OutputName) { $OutputName = "lenteuv_portable_$Version" }
if ($OutputName -notmatch '^[A-Za-z0-9][A-Za-z0-9._-]*$') { throw 'OutputName must be a plain filename, without directories.' }
if (!$SkipBuild) { & (Join-Path $PSScriptRoot 'build.ps1') -Configuration Release -CMakePath $CMakePath }
$binary = Join-Path $ProjectRoot 'build/windows-x64/bin/Release/lenteuv.exe'
if (!(Test-Path -LiteralPath $binary)) { throw 'Release executable not found. Run scripts/build.ps1 first.' }

$vsRoot = $null
if (!$RuntimeDirectory -or !$DumpBinPath) { $vsRoot = Get-VisualStudioRoot }
if (!$RuntimeDirectory) {
    $runtimeVersion = Get-ChildItem -LiteralPath (Join-Path $vsRoot 'VC/Redist/MSVC') -Directory | Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' } |
        Sort-Object { [version]$_.Name } -Descending | Where-Object { Test-Path (Join-Path $_.FullName 'x64/Microsoft.VC143.CRT') } | Select-Object -First 1
    if (!$runtimeVersion) { throw 'VC++ x64 app-local runtime not found. Supply -RuntimeDirectory.' }
    $RuntimeDirectory = Join-Path $runtimeVersion.FullName 'x64/Microsoft.VC143.CRT'
}
if (!$DumpBinPath) {
    $toolsVersion = Get-ChildItem -LiteralPath (Join-Path $vsRoot 'VC/Tools/MSVC') -Directory | Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' } | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
    $DumpBinPath = Resolve-Tool 'dumpbin.exe' '' @((Join-Path $toolsVersion.FullName 'bin/Hostx64/x64/dumpbin.exe'))
} else { $DumpBinPath = Resolve-Tool 'dumpbin.exe' $DumpBinPath }
if ($Format -in @('Installer', 'All')) {
    $ISCCPath = Resolve-Tool 'ISCC.exe' $ISCCPath @(
        "$env:LOCALAPPDATA/Programs/Inno Setup 6/ISCC.exe",
        "$env:USERPROFILE/AppData/Local/Programs/Inno Setup 6/ISCC.exe",
        "${env:ProgramFiles(x86)}/Inno Setup 6/ISCC.exe",
        "$env:ProgramFiles/Inno Setup 6/ISCC.exe"
    )
}

$stagingParent = Join-Path $ProjectRoot 'build/staging'
$stage = Assert-ChildPath (Join-Path $stagingParent ([guid]::NewGuid().ToString('N'))) $stagingParent
$dist = Join-Path $ProjectRoot 'dist'
New-Item -ItemType Directory -Path $stage, $dist -Force | Out-Null
Copy-Item -LiteralPath $binary -Destination $stage
Copy-Item -LiteralPath (Join-Path $ProjectRoot 'src/resources/lenteuv.ico'), (Join-Path $ProjectRoot 'LICENSE') -Destination $stage
foreach ($locale in Get-ChildItem -LiteralPath (Join-Path $ProjectRoot 'locales') -Directory) {
    $catalog = Join-Path $locale.FullName 'LC_MESSAGES/base.po'
    if (Test-Path -LiteralPath $catalog) {
        $target = Join-Path $stage "locales/$($locale.Name)/LC_MESSAGES"
        New-Item -ItemType Directory -Path $target -Force | Out-Null
        Copy-Item -LiteralPath $catalog -Destination $target
    }
}
foreach ($dll in @('msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')) {
    if (!(Test-Path -LiteralPath (Join-Path $RuntimeDirectory $dll))) { throw "Missing runtime DLL: $dll" }
}
Get-ChildItem -LiteralPath $RuntimeDirectory -Filter '*.dll' | Copy-Item -Destination $stage
$userData = Join-Path $stage 'user_data'
New-Item -ItemType Directory -Path $userData -Force | Out-Null
foreach ($subdir in @('boards', 'cache', 'tags')) { New-Item -ItemType Directory -Path (Join-Path $userData $subdir) -Force | Out-Null }
Copy-Item -LiteralPath (Join-Path $ProjectRoot 'packaging/default-settings.json') -Destination (Join-Path $userData 'settings.json')
[IO.File]::WriteAllText((Join-Path $userData 'session.txt'), '', [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText((Join-Path $stage 'README.txt'), "LenteUV $Version`r`nRun lenteuv.exe with an image or folder path. Windows 10 1903+ x64; optional formats depend on installed WIC codecs.`r`nData is stored in user_data beside the executable. Keep the extracted folder writable.`r`nGNU GPL: see LICENSE. Sources and attribution: https://github.com/javelloj-gif/xph_image_viewer (based on Sergei Krumas' image_viewer, extended by xPhang).`r`n", [Text.UTF8Encoding]::new($false))

# Static imports are checked for the executable and all app-local runtime DLLs.
# This does not validate GPU initialization or the user interface.
foreach ($pe in Get-ChildItem -LiteralPath $stage -File | Where-Object { $_.Extension -in @('.exe', '.dll') }) {
    $imports = & $DumpBinPath /dependents $pe.FullName
    Assert-NativeSuccess "Inspecting $($pe.Name)"
    foreach ($line in $imports) {
        if ($line -match '^\s+([^\s]+\.dll)\s*$') {
            $name = $Matches[1]
            if ($name -match '^(api-ms-|ext-ms-)') { continue }
            if (!(Test-Path -LiteralPath (Join-Path $stage $name)) -and !(Test-Path -LiteralPath (Join-Path "$env:SystemRoot/System32" $name))) {
                throw "Unresolved import $name in $($pe.Name). Staging retained: $stage"
            }
        }
    }
}
if ($Format -in @('Portable', 'All')) {
    $zip = Assert-ChildPath (Join-Path $dist "$OutputName.zip") $dist
    if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip -Force }
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [IO.Compression.ZipFile]::CreateFromDirectory($stage, $zip, [IO.Compression.CompressionLevel]::Optimal, $false)
    if (!(Test-Path -LiteralPath $zip)) { throw 'ZIP was not generated.' }
    Write-Host "Portable: $zip"
}
if ($Format -in @('Installer', 'All')) {
    $installer = Assert-ChildPath (Join-Path $dist "lenteuv_setup_$Version.exe") $dist
    if (Test-Path -LiteralPath $installer) { Remove-Item -LiteralPath $installer -Force }
    & $ISCCPath "/DStageDir=$stage" "/DMyAppVersion=$Version" "/DMyOutputDir=$dist" (Join-Path $ProjectRoot 'packaging/lenteuv_setup.iss')
    Assert-NativeSuccess 'Inno Setup'
    if (!(Test-Path -LiteralPath $installer)) { throw 'Installer was not generated.' }
    Write-Host "Installer: $installer"
}
Write-Host "Validated staging retained for inspection: $stage"
