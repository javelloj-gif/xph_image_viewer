#Requires -Version 5.1
# Elevated ETW-only helper. Never launches LenteUV; owns only its named WPR instance.
param(
    [Parameter(Mandatory)][string]$RunRoot,
    [Parameter(Mandatory)][ValidatePattern('^LenteUV003-[a-f0-9]{32}$')][string]$Instance
)
$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
$run=[IO.Path]::GetFullPath($RunRoot)
if (!$run.StartsWith(([IO.Path]::GetFullPath("$project/build/").TrimEnd('\','/')+'\'),[StringComparison]::OrdinalIgnoreCase)) {
    throw 'Collector output must be inside project/build.'
}
if (!(Test-Path -LiteralPath "$run/request.json" -PathType Leaf)) { throw 'Missing collector request.' }
$request=Get-Content -LiteralPath "$run/request.json" -Raw | ConvertFrom-Json
if ($request.Instance -ne $Instance) { throw 'Collector request does not match instance.' }
$wpr="$env:SystemRoot/System32/wpr.exe"
$ownsRecording=$false
$outcome=[ordered]@{Instance=$Instance;Status='starting';StartExitCode=$null;StopExitCode=$null;Error=$null}
# Publishes complete handshake JSON atomically; parent never accepts a partial ready file.
function Write-CollectorState([string]$Name,[object]$Value) {
    $temporary=Join-Path $run ($Name+'.tmp')
    [IO.File]::WriteAllText($temporary,($Value | ConvertTo-Json -Depth 6),[Text.UTF8Encoding]::new($false))
    Move-Item -LiteralPath $temporary -Destination (Join-Path $run $Name) -Force
}
try {
    $principal=[Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
    if (!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) { throw 'ETW collector requires elevation.' }
    if (Test-Path -LiteralPath "$run/stop.request") { throw 'Parent cancelled before collector startup.' }
    New-Item -ItemType Directory -Path "$run/temp" -ErrorAction Stop | Out-Null
    & $wpr -start GeneralProfile.Verbose -start FileIO.Verbose -filemode -recordtempto "$run/temp" -instancename $Instance 2>&1 |
        Out-File -LiteralPath "$run/wpr-start.txt" -Encoding utf8
    $outcome.StartExitCode=$LASTEXITCODE
    if ($LASTEXITCODE -ne 0) { throw "WPR start failed: $LASTEXITCODE. No target launch allowed." }
    $ownsRecording=$true
    & $wpr -status profiles collectors -details -instancename $Instance 2>&1 |
        Out-File -LiteralPath "$run/wpr-active.txt" -Encoding utf8
    if ($LASTEXITCODE -ne 0) { throw 'Named WPR recording could not be confirmed.' }
    $active=Get-Content -LiteralPath "$run/wpr-active.txt" -Raw
    foreach ($keyword in @('CSwitch','ReadyThread','SampledProfile','Loader','HardFaults','DiskIO','FileIO')) {
        if ($active -notmatch ('(?m)^\s*'+[regex]::Escape($keyword)+'\s*$')) { throw "Required ETW keyword missing before readiness: $keyword" }
    }
    Write-CollectorState 'ready.json' @{Instance=$Instance;ReadyUtc=[DateTimeOffset]::UtcNow.ToString('o');Administrator=$true;Status='recording';Pid=$PID}
    $deadline=[DateTime]::UtcNow.AddSeconds(90)
    while (!(Test-Path -LiteralPath "$run/stop.request")) {
        if ([DateTime]::UtcNow -gt $deadline) { $outcome.Status='watchdog'; break }
        Start-Sleep -Milliseconds 100
    }
    if ($outcome.Status -ne 'watchdog') { $outcome.Status='stopping' }
} catch {
    $outcome.Status='failed'; $outcome.Error=$_.Exception.Message
} finally {
    if ($ownsRecording) {
        # Stop/merge only our instance, including on parent loss; retain ETL on failures.
        & $wpr -stop "$run/startup.etl" 'LenteUV 003 startup diagnosis' -skipPdbGen -instancename $Instance 2>&1 |
            Out-File -LiteralPath "$run/wpr-stop.txt" -Encoding utf8
        $outcome.StopExitCode=$LASTEXITCODE
        if ($LASTEXITCODE -eq 0) {
            if ($outcome.Status -eq 'stopping') { $outcome.Status='saved' }
        } else {
            $outcome.Status='stop-failed'
            # Release only the owned instance after preserving failed-stop diagnostics/temp files.
            & $wpr -cancel -instancename $Instance 2>&1 | Out-File -LiteralPath "$run/wpr-cancel.txt" -Encoding utf8
        }
    }
    $outcome.FinishedUtc=[DateTimeOffset]::UtcNow.ToString('o')
    $outcome.EtlExists=Test-Path -LiteralPath "$run/startup.etl" -PathType Leaf
    Write-CollectorState 'collector-result.json' $outcome
}
if ($outcome.Status -ne 'saved') { exit 1 }
