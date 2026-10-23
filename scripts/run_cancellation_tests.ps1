param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [Parameter(Mandatory = $true)][string]$Fixtures,
    [Parameter(Mandatory = $true)][string]$RunRoot
)
$ErrorActionPreference = 'Stop'
$cancellationRun = [IO.Path]::GetFullPath((Join-Path $RunRoot ('cancel-' + [guid]::NewGuid().ToString('N'))))
New-Item -ItemType Directory -Path (Join-Path $cancellationRun 'test'), (Join-Path $cancellationRun 'user_data') | Out-Null
Copy-Item -LiteralPath $Executable -Destination (Join-Path $cancellationRun 'cancellation_tests.exe')
Copy-Item -LiteralPath (Join-Path $Fixtures 'linux.png') -Destination (Join-Path $cancellationRun 'test/seed.png')
Push-Location $cancellationRun
try {
    & (Join-Path $cancellationRun 'cancellation_tests.exe') --cancellation-tests
    $cancellationExit = $LASTEXITCODE
} finally { Pop-Location }
Write-Host "Isolated cancellation artifacts: $cancellationRun"
exit $cancellationExit
