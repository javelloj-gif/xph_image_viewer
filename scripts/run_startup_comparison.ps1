# Sequential native matrix; never overlaps two benchmark viewers or touches source photographs for writes.
param(
    [Parameter(Mandatory)][string]$ReferenceExecutable,
    [Parameter(Mandatory)][string]$ReferenceRevision,
    [Parameter(Mandatory)][string]$CandidateExecutable,
    [string]$CandidateRevision,
    [Parameter(Mandatory)][string]$Images,
    [Parameter(Mandatory)][string]$RunRoot,
    [ValidateRange(1,100)][int]$Repetitions=30,
    [ValidateRange(1,30)][int]$ColdRepetitions=30
)
$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
$root=[IO.Path]::GetFullPath($RunRoot)
if (!$root.StartsWith("$project\build\",[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $root)) { throw 'RunRoot must be a new directory inside build.' }
New-Item -ItemType Directory -Path $root | Out-Null
$candidateSource=if ($CandidateRevision) { $CandidateRevision } else { & git -C $project rev-parse HEAD }
$variants=@{
    base=@{Executable=$ReferenceExecutable;SourceRevision=$ReferenceRevision}
    candidate=@{Executable=$CandidateExecutable;SourceRevision=$candidateSource}
}
$scenarioIndex=0
foreach ($scenario in @('viewer','strip','waterfall')) {
    $order=if ($scenarioIndex%2) { @('candidate','base') } else { @('base','candidate') }
    foreach ($variant in $order) {
        Write-Host "Starting $variant/$scenario, $Repetitions measured opens per position"
        $variantOptions=$variants[$variant]
        & "$PSScriptRoot\run_startup_benchmark.ps1" @variantOptions -Images $Images -RunRoot "$root\$variant-$scenario-warm" -Scenario $scenario -Cache warm -Repetitions $Repetitions
    }
    ++$scenarioIndex
}
foreach ($scenario in @('viewer','strip','waterfall')) {
    Write-Host "Starting candidate/$scenario with a fresh cache namespace per trial"
    $candidateOptions=$variants.candidate
    & "$PSScriptRoot\run_startup_benchmark.ps1" @candidateOptions -Images $Images -RunRoot "$root\candidate-$scenario-empty" -Scenario $scenario -Cache empty -Repetitions $ColdRepetitions
}
foreach ($variant in @('base','candidate')) {
    Write-Host "Starting separate $variant resource samples (three 15-second processes)"
    $resourceOptions=$variants[$variant]
    & "$PSScriptRoot\run_startup_benchmark.ps1" @resourceOptions -Images $Images -RunRoot "$root\$variant-resources" -Scenario viewer -Cache warm -Repetitions 1 -Warmups 0 -Resources
}
Write-Host "Comparison completed: $root"
