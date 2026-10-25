#Requires -Version 7.0
# Prepares or validates ETW without the product; Capture explicitly launches it once at normal integrity.
[CmdletBinding()]
param(
    [ValidateSet('Preflight','Prepare','ValidateCollector','Capture')][string]$Mode='Preflight',
    [switch]$ConfirmFirstLaunch
)
$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
$state=Join-Path $project 'build/startup-etw-003-preparation'
$candidate=Join-Path $project 'dist/lenteuv_qa_003_20261024_async_cache/lenteuv.exe'
$target='F:\Pictures\FotosTEMP'
$expectedHash='A8C54E3B42B5811EA17CCF347AA91A7FFF4C2808BF19EE45BD4E6C7F0AF04213'
$preparationPath=Join-Path $state 'preparation.json'
$principal=[Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
$isAdministrator=$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
$boot=(Get-CimInstance Win32_OperatingSystem).LastBootUpTime.ToUniversalTime()
$bootId=$boot.Ticks.ToString()
$wpr="$env:SystemRoot/System32/wpr.exe"
$wpt="${env:ProgramFiles(x86)}/Windows Kits/10/Windows Performance Toolkit"
$exeDirectory=Split-Path $candidate -Parent
$paths=@($candidate,"$exeDirectory/user_data/settings.json","$exeDirectory/user_data/session.txt")
New-Item -ItemType Directory -Path $state -Force | Out-Null
# Captures metadata only in preflight/capture; never hashes or copies candidate pages before the first launch.
function Get-InputStamp([string]$Path) {
    $item=Get-Item -LiteralPath $Path
    [pscustomobject]@{Path=$item.FullName;Length=$item.Length;LastWriteUtcTicks=$item.LastWriteTimeUtc.Ticks}
}
# Writes local manifests; paths/private state never enter versioned documents.
function Write-LocalJson([string]$Path,[object]$Value) {
    [IO.File]::WriteAllText($Path,($Value | ConvertTo-Json -Depth 10),[Text.UTF8Encoding]::new($false))
}
# Uses one exclusive file creation for the boot guard; distinct RunRoots cannot permit another target launch.
function New-BootAttempt([string]$Path) {
    $guard=[IO.File]::Open($Path,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
    $guard.Dispose()
}
foreach ($tool in @($wpr,"$wpt/wpa.exe","$wpt/xperf.exe")) {
    if (!(Test-Path -LiteralPath $tool -PathType Leaf)) { throw "Missing diagnostic tool: $tool" }
}
foreach ($path in $paths) { if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing candidate input: $path" } }
if (!(Test-Path -LiteralPath $target -PathType Container)) { throw 'The agreed media directory is unavailable.' }
$running=@(Get-Process -Name lenteuv -ErrorAction SilentlyContinue)
if ($Mode -eq 'Prepare') {
    if ($running.Count) { throw 'Close LenteUV normally before preparation.' }
    if ((Get-FileHash -LiteralPath $candidate).Hash -ne $expectedHash) { throw 'Unexpected candidate hash.' }
    $settings=Get-Content -LiteralPath $paths[1] -Raw | ConvertFrom-Json
    if ($settings.settings.page_on_app_launch_from_explorer[0] -ne 2 -or $settings.settings.hide_on_app_start[0] -or
        $settings.settings.enable_file_log[0] -or $settings.settings.debug_ipc[0]) {
        throw 'Preparation requires the existing visible Viewer configuration with logging disabled. No settings changed.'
    }
    $snapshot=Join-Path $state ('pinned-'+[guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $snapshot | Out-Null
    Copy-Item -LiteralPath $paths[1] -Destination "$snapshot/settings.json"
    Copy-Item -LiteralPath $paths[2] -Destination "$snapshot/session.txt"
    $prepared=[ordered]@{
        PreparedUtc=[DateTimeOffset]::UtcNow.ToString('o');PreparedBootId=$bootId
        Candidate=$candidate;CandidateHash=$expectedHash;Target=$target;Arguments=@($target)
        Inputs=@($paths | ForEach-Object { Get-InputStamp $_ });Snapshot=$snapshot
        WprVersion=(Get-Item -LiteralPath $wpr).VersionInfo.FileVersion
        CollectorValidated=$false;CaptureBootGuard='attempt-<boot-ticks>.json'
        Scenario='Explicit directory in Viewer; preserve current cache/session. Media inputs read only.'
    }
    Write-LocalJson $preparationPath $prepared
    'Prepared. No product process launched. ValidateCollector must pass before Capture.'
    exit
}
if (!(Test-Path -LiteralPath $preparationPath)) { throw 'Run -Mode Prepare before the next fresh session.' }
$prepared=Get-Content -LiteralPath $preparationPath -Raw | ConvertFrom-Json
if ($prepared.Candidate -ne $candidate -or $prepared.Target -ne $target) { throw 'Preparation does not match the fixed candidate/target.' }
foreach ($pinnedInput in $prepared.Inputs) {
    $current=Get-InputStamp $pinnedInput.Path
    if ($current.Length -ne $pinnedInput.Length -or $current.LastWriteUtcTicks -ne $pinnedInput.LastWriteUtcTicks) {
        throw 'Candidate/settings/session changed after preparation. Stop before launching; prepare again before a future fresh session.'
    }
}
if ((Get-Item -LiteralPath $wpr).VersionInfo.FileVersion -ne $prepared.WprVersion) { throw 'WPR version changed; revalidate before any product launch.' }
if ($Mode -eq 'Preflight') {
    [pscustomobject]@{Mode=$Mode;BootId=$bootId;PreparedBootId=$prepared.PreparedBootId;NewBoot=($bootId -ne $prepared.PreparedBootId);CollectorValidated=$prepared.CollectorValidated;Administrator=$isAdministrator;ExistingLenteuvCount=$running.Count;Target=$target;LaunchesProduct=$false} | ConvertTo-Json
    exit
}
if ($isAdministrator) { throw 'Run the orchestrator without elevation. Only its ETW helper may be elevated.' }
if ($running.Count) { throw 'Existing LenteUV process detected; no capture launch allowed.' }
if ([IO.DriveInfo]::new([IO.Path]::GetPathRoot($state)).AvailableFreeSpace -lt 2GB) { throw 'At least 2 GiB free on the output volume is required for trace/merge. Target not launched.' }
if ($Mode -eq 'Capture') {
    if (!$ConfirmFirstLaunch) { throw 'Owner confirmation that LenteUV has not run in this session is required (-ConfirmFirstLaunch).' }
    if ($bootId -eq $prepared.PreparedBootId) { throw 'This is the preparation session, whose first opportunity was already consumed. No target launched.' }
    if (!$prepared.CollectorValidated) { throw 'The collector has not passed validation. No target launched.' }
    if ((Get-FileHash -LiteralPath $PSCommandPath).Hash -ne $prepared.OrchestratorHash -or
        (Get-FileHash -LiteralPath "$PSScriptRoot/startup_etw_collector.ps1").Hash -ne $prepared.CollectorHash) {
        throw 'Capture scripts changed after collector validation. Revalidate before any launch.'
    }
    if (Test-Path -LiteralPath "$state/attempt-$bootId.json") { throw 'A launch attempt is already recorded for this boot. Never retry.' }
}
$run=Join-Path $project ('build/startup-etw-003-'+$Mode.ToLowerInvariant()+'-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $run | Out-Null
$instance='LenteUV003-'+[guid]::NewGuid().ToString('N')
Write-LocalJson "$run/request.json" @{Instance=$instance;Mode=$Mode;Target=$target;Candidate=$candidate;BootId=$bootId}
$helper=Join-Path $PSScriptRoot 'startup_etw_collector.ps1'
$collector=$null
$child=$null
$capture=[ordered]@{Mode=$Mode;RunRoot=$run;Instance=$instance;BootUtc=$boot.ToString('o');Target=$target;ProductLaunchAttempted=$false;ElevatedProduct=$false;Error=$null}
try {
    $helperArguments=@('-NoProfile','-NonInteractive','-ExecutionPolicy','Bypass','-File',('"'+$helper+'"'),'-RunRoot',('"'+$run+'"'),'-Instance',$instance)
    $collector=Start-Process -FilePath "$env:SystemRoot/System32/WindowsPowerShell/v1.0/powershell.exe" -Verb RunAs -WindowStyle Hidden -ArgumentList $helperArguments -PassThru
    $readyDeadline=[DateTime]::UtcNow.AddSeconds(60)
    while (!(Test-Path -LiteralPath "$run/ready.json")) {
        if ((Test-Path -LiteralPath "$run/collector-result.json") -or $collector.HasExited) { throw 'ETW helper failed before readiness. Inspect collector logs; target not launched.' }
        if ([DateTime]::UtcNow -gt $readyDeadline) { throw 'ETW readiness timeout. Target not launched.' }
        Start-Sleep -Milliseconds 100
    }
    $ready=Get-Content -LiteralPath "$run/ready.json" -Raw | ConvertFrom-Json
    if ($ready.Instance -ne $instance -or !$ready.Administrator -or $ready.Status -ne 'recording') { throw 'Invalid ETW handshake. Target not launched.' }
    if ($Mode -eq 'ValidateCollector') {
        # Exercise the real collector for a short bounded interval, never the image viewer or media.
        Start-Sleep -Seconds 3
    } else {
        if (@(Get-Process -Name lenteuv -ErrorAction SilentlyContinue).Count) { throw 'LenteUV appeared while awaiting ETW readiness. Target not launched.' }
        if ($collector.HasExited) { throw 'Collector exited before launch. Target not launched.' }
        foreach ($pinnedInput in $prepared.Inputs) {
            $current=Get-InputStamp $pinnedInput.Path
            if ($current.Length -ne $pinnedInput.Length -or $current.LastWriteUtcTicks -ne $pinnedInput.LastWriteUtcTicks) {
                throw 'Candidate/settings/session changed while awaiting ETW. Target not launched.'
            }
        }
        Add-Type -TypeDefinition @'
using System.Runtime.InteropServices;
// QPC/FILETIME correlation only; no media writes or process injection.
public static class StartupEtw003Clock {
 [DllImport("kernel32.dll")] public static extern void GetSystemTimePreciseAsFileTime(out long value);
}
'@
        $info=[Diagnostics.ProcessStartInfo]::new($candidate)
        $info.UseShellExecute=$false; $info.WorkingDirectory=$exeDirectory
        $info.WindowStyle=[Diagnostics.ProcessWindowStyle]::Normal
        $info.ArgumentList.Add($target)
        foreach ($key in @($info.Environment.Keys | Where-Object { $_ -like 'LENTEUV_PERF_*' -or $_ -eq 'LENTEUV_CACHE_NAMESPACE' })) { [void]$info.Environment.Remove($key) }
        $info.Environment['LENTEUV_PERF_OUTPUT']="$run/qpc.csv"
        $info.Environment['LENTEUV_PERF_SCENARIO']='viewer'
        $info.Environment['LENTEUV_PERF_EXIT_MS']='15000'
        $info.Environment['LENTEUV_PERF_READY_EXIT_MS']='2500'
        $capture.Frequency=[Diagnostics.Stopwatch]::Frequency
        $capture.SyncBeforeQpc=[Diagnostics.Stopwatch]::GetTimestamp()
        $syncTime=0L; [StartupEtw003Clock]::GetSystemTimePreciseAsFileTime([ref]$syncTime)
        $capture.SyncAfterQpc=[Diagnostics.Stopwatch]::GetTimestamp()
        $capture.SyncFileTime=$syncTime
        New-BootAttempt "$state/attempt-$bootId.json"
        Write-LocalJson "$state/attempt-$bootId.json" @{RunRoot=$run;Instance=$instance;BootId=$bootId;NeverRetry=$true}
        $capture.ProductLaunchAttempted=$true
        $capture.InvocationUtc=[DateTimeOffset]::UtcNow.ToString('o')
        $capture.InvocationQpc=[Diagnostics.Stopwatch]::GetTimestamp()
        # The sole product launch site: direct fixed EXE + fixed folder, inheriting the normal token.
        $child=[Diagnostics.Process]::Start($info)
        $capture.StartReturnedQpc=[Diagnostics.Stopwatch]::GetTimestamp()
        $capture.ProductPid=$child.Id
        $capture.CreationFileTime=$child.StartTime.ToUniversalTime().ToFileTimeUtc()
        Write-LocalJson "$run/capture.json" $capture
        $end=[DateTime]::UtcNow.AddSeconds(45)
        while (!$child.WaitForExit(100)) {
            if ([DateTime]::UtcNow -gt $end) {
                [void]$child.CloseMainWindow()
                throw 'Own product exceeded 45 s. Normal close requested; preserve partial trace and do not retry.'
            }
        }
        $capture.ExitCode=$child.ExitCode
        $capture.ExitObservedQpc=[Diagnostics.Stopwatch]::GetTimestamp()
    }
} catch {
    $capture.Error=$_.Exception.Message
} finally {
    [IO.File]::WriteAllText("$run/stop.request",'stop only the owned named collector')
    if ($collector) {
        # Bounded wait; helper has its own watchdog if the parent disappears.
        $collector.WaitForExit(55000) | Out-Null
        $collector.Dispose()
    }
    if ($child) { $child.Dispose() }
    Write-LocalJson "$run/capture.json" $capture
}
if ($capture.Error) { throw "$($capture.Error) Artifacts: $run" }
if (!(Test-Path -LiteralPath "$run/collector-result.json")) { throw "Collector did not finish merging; do not relaunch. Artifacts: $run" }
$outcome=Get-Content -LiteralPath "$run/collector-result.json" -Raw | ConvertFrom-Json
if ($outcome.Status -ne 'saved' -or !$outcome.EtlExists) { throw "ETW was not saved correctly; do not relaunch. Artifacts: $run" }
& "$wpt/xperf.exe" -i "$run/startup.etl" -a tracestats 2>&1 | Out-File -LiteralPath "$run/trace-statistics.txt" -Encoding utf8
if ($LASTEXITCODE -ne 0) { throw "ETL statistics failed. Preserve trace; do not relaunch. Artifacts: $run" }
$statistics=Get-Content -LiteralPath "$run/trace-statistics.txt" -Raw
$lostBuffers=[regex]::Match($statistics,'Total # Lost Buffers\s*:\s*(\d+)')
$lostEvents=[regex]::Match($statistics,'Total # Lost Events\s*:\s*(\d+)')
if (!$lostBuffers.Success -or !$lostEvents.Success -or [long]$lostBuffers.Groups[1].Value -ne 0 -or [long]$lostEvents.Groups[1].Value -ne 0) {
    throw "Trace loss is nonzero or unverified. Preserve evidence without relaunching: $run"
}
$active=Get-Content -LiteralPath "$run/wpr-active.txt" -Raw
foreach ($keyword in @('CSwitch','ReadyThread','SampledProfile','Loader','HardFaults','DiskIO','FileIO')) {
    if ($active -notmatch ('(?m)^\s*'+[regex]::Escape($keyword)+'\s*$')) { throw "Missing required ETW keyword $keyword. Preserve trace: $run" }
}
if ($Mode -eq 'ValidateCollector') {
    $prepared.CollectorValidated=$true
    $prepared | Add-Member -NotePropertyName ValidationRun -NotePropertyValue $run -Force
    $prepared | Add-Member -NotePropertyName ValidationUtc -NotePropertyValue ([DateTimeOffset]::UtcNow.ToString('o')) -Force
    $prepared | Add-Member -NotePropertyName OrchestratorHash -NotePropertyValue (Get-FileHash -LiteralPath $PSCommandPath).Hash -Force
    $prepared | Add-Member -NotePropertyName CollectorHash -NotePropertyValue (Get-FileHash -LiteralPath "$PSScriptRoot/startup_etw_collector.ps1").Hash -Force
    Write-LocalJson $preparationPath $prepared
} else {
    # Full reads/hashes are deferred until after product exit; never prewarm the EXE before capture.
    $capture.ExecutableHashAfter=(Get-FileHash -LiteralPath $candidate).Hash
    $capture.ExpectedHash=$expectedHash
    $capture.SettingsHashAfter=(Get-FileHash -LiteralPath $paths[1]).Hash
    $capture.SessionHashAfter=(Get-FileHash -LiteralPath $paths[2]).Hash
    $capture.SettingsUnchanged=($capture.SettingsHashAfter -eq (Get-FileHash -LiteralPath "$($prepared.Snapshot)/settings.json").Hash)
    $capture.SessionUnchanged=($capture.SessionHashAfter -eq (Get-FileHash -LiteralPath "$($prepared.Snapshot)/session.txt").Hash)
    Write-LocalJson "$run/capture.json" $capture
    if ($capture.ExecutableHashAfter -ne $expectedHash) { throw 'Measured EXE hash differs from prepared candidate; keep evidence without retry.' }
    $events=@(Import-Csv -LiteralPath "$run/qpc.csv")
    if ($capture.ExitCode -ne 0 -or @($events | Where-Object phase -eq 'main.entry').Count -ne 1 -or
        @($events | Where-Object phase -eq 'frame.original.submitted').Count -lt 1 -or
        @($events | Where-Object { $_.phase -eq 'trace.dropped' -and [long]$_.value -ne 0 }).Count) {
        throw 'QPC original/entry/exit checks failed. Preserve partial evidence; never relaunch this boot.'
    }
}
[pscustomobject]@{Mode=$Mode;RunRoot=$run;EtlSaved=$true;ProductLaunchAttempted=$capture.ProductLaunchAttempted;CollectorValidated=$prepared.CollectorValidated} | ConvertTo-Json
