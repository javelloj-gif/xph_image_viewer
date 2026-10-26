param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [Parameter(Mandatory = $true)][string]$Fixtures,
    [Parameter(Mandatory = $true)][string]$RunRoot,
    [string]$ReferenceExecutable,
    [switch]$Startup
)
$ErrorActionPreference = 'Stop'
$ipcRunDirectory = [IO.Path]::GetFullPath((Join-Path $RunRoot ('ipc-' + [guid]::NewGuid().ToString('N'))))
New-Item -ItemType Directory -Path (Join-Path $ipcRunDirectory 'test'), (Join-Path $ipcRunDirectory 'user_data') | Out-Null
Copy-Item -LiteralPath $Executable -Destination (Join-Path $ipcRunDirectory 'ipc_diagnostic_tests.exe')
if ($ReferenceExecutable) {
    Copy-Item -LiteralPath $ReferenceExecutable -Destination (Join-Path $ipcRunDirectory 'reference-tests.exe')
}
Copy-Item -LiteralPath (Join-Path $Fixtures 'noname.jpg') -Destination (Join-Path $ipcRunDirectory 'test/seed.jpg')
Copy-Item -LiteralPath (Join-Path $Fixtures 'linux.png') -Destination (Join-Path $ipcRunDirectory ('test/imagen ' + [char]0x03A9 + '.png'))
Push-Location $ipcRunDirectory
try {
    if ($Startup) {
        # Explicit native-device smoke only; default CTest remains WARP/hidden and requires no hardware GPU.
        $startupEndpoint = '\\.\pipe\image-viewer-startup-' + [guid]::NewGuid().ToString('N')
        & (Join-Path $ipcRunDirectory 'ipc_diagnostic_tests.exe') --ipc-startup $startupEndpoint (Join-Path $ipcRunDirectory 'test/seed.jpg')
    } else {
        & (Join-Path $ipcRunDirectory 'ipc_diagnostic_tests.exe') --ipc-diagnostics
    }
    $ipcExitCode = $LASTEXITCODE
} finally {
    Pop-Location
}
Write-Host "Isolated IPC diagnostic artifacts: $ipcRunDirectory"
exit $ipcExitCode
