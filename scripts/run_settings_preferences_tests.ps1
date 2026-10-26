# Copies the harness and defaults into a unique EXE-local data tree before starting Settings.
param([Parameter(Mandatory)][string]$Executable,
      [Parameter(Mandatory)][string]$Defaults,
      [Parameter(Mandatory)][string]$RunRoot)
$ErrorActionPreference='Stop'
$run=Join-Path ([IO.Path]::GetFullPath($RunRoot)) ('settings-preferences-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path "$run\user_data" | Out-Null
New-Item -ItemType File -Path "$run\preferences-test.marker" | Out-Null
Copy-Item -LiteralPath $Executable -Destination "$run\preferences_tests.exe"
Copy-Item -LiteralPath $Defaults -Destination "$run\default-settings.json"
Push-Location $run
try { & "$run\preferences_tests.exe" --settings-preferences-tests; $code=$LASTEXITCODE }
finally { Pop-Location }
Write-Host "Isolated preference artifacts: $run"
exit $code
