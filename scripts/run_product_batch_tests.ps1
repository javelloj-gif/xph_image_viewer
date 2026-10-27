# Executes production UI/model regressions with executable-local synthetic files and preferences.
param([Parameter(Mandatory)][string]$Executable,[Parameter(Mandatory)][string]$RunRoot)
$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
$root=[IO.Path]::GetFullPath($RunRoot)
if (!$root.StartsWith("$project\build\",[StringComparison]::OrdinalIgnoreCase)) { throw 'RunRoot must be inside build.' }
$run=Join-Path $root ("product-batch-"+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path "$run\user_data" -Force | Out-Null
Copy-Item -LiteralPath $Executable -Destination "$run\product-batch-tests.exe"
Get-ChildItem -LiteralPath (Split-Path $Executable) -Filter *.dll -File | Copy-Item -Destination $run
Copy-Item -LiteralPath "$project\locales" -Destination "$run\locales" -Recurse
Push-Location $run
try { & "$run\product-batch-tests.exe" --product-batch-tests; if ($LASTEXITCODE) { throw "Product batch tests failed: $LASTEXITCODE" } }
finally { Pop-Location }
