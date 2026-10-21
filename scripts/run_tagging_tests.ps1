param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [Parameter(Mandatory = $true)][string]$Fixtures,
    [Parameter(Mandatory = $true)][string]$RunRoot
)
$ErrorActionPreference = 'Stop'
$runDir = Join-Path $RunRoot ([guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path (Join-Path $runDir 'test'), (Join-Path $runDir 'user_data') -Force | Out-Null
Copy-Item -LiteralPath $Executable -Destination (Join-Path $runDir 'tagging_tests.exe')
# The harness intentionally changes metadata. Only copies may be passed to it.
Copy-Item -LiteralPath (Join-Path $Fixtures 'noname.jpg'), (Join-Path $Fixtures 'linux.png') -Destination (Join-Path $runDir 'test')
Push-Location $runDir
try {
    & (Join-Path $runDir 'tagging_tests.exe') 2>&1 | ForEach-Object { $_.ToString().Replace([string][char]0, '') }
    $testExitCode = $LASTEXITCODE
} finally {
    Pop-Location
}
Write-Host "Isolated test artifacts: $runDir"
exit $testExitCode
