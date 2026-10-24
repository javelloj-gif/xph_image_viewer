# Own EXE/user_data and working directory: no ancestor/personal session data can be discovered.
param([Parameter(Mandatory)][string]$Executable,[Parameter(Mandatory)][string]$RunRoot)
$ErrorActionPreference='Stop'
$run=Join-Path ([IO.Path]::GetFullPath($RunRoot)) ('startup-cache-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path "$run\user_data" | Out-Null
Copy-Item -LiteralPath $Executable -Destination "$run\startup_cache_tests.exe"
Push-Location $run
try { & "$run\startup_cache_tests.exe" --startup-cache-tests; $code=$LASTEXITCODE }
finally { Pop-Location }
Write-Host "Isolated startup/cache artifacts: $run"
exit $code
