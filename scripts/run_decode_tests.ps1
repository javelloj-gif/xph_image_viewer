param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [Parameter(Mandatory = $true)][string]$Fixtures,
    [Parameter(Mandatory = $true)][string]$RunRoot
)
$ErrorActionPreference = 'Stop'
$decodeRunDirectory = Join-Path $RunRoot ('decode-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path (Join-Path $decodeRunDirectory 'fixtures'), (Join-Path $decodeRunDirectory 'user_data') | Out-Null
Copy-Item -LiteralPath $Executable -Destination (Join-Path $decodeRunDirectory 'decode_tests.exe')
foreach ($decodeFixture in @('noname.jpg','apng.apng','apng_png.png','static.webp','animated.webp','rubik.gif')) {
    Copy-Item -LiteralPath (Join-Path $Fixtures $decodeFixture) -Destination (Join-Path $decodeRunDirectory 'fixtures')
}
Push-Location $decodeRunDirectory
try {
    & (Join-Path $decodeRunDirectory 'decode_tests.exe') --decode-tests
    $decodeExitCode = $LASTEXITCODE
} finally {
    Pop-Location
}
Write-Host "Isolated decoder artifacts: $decodeRunDirectory"
exit $decodeExitCode
