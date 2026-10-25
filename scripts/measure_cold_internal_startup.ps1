#Requires -Version 7.0
# One new-candidate launch per boot, using the pre-existing portable staging and validated ETW helper.
[CmdletBinding()]
param([switch]$ConfirmFirstLaunch)
$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
$state = Join-Path $project 'build/startup-etw-003-preparation'
$candidate = Join-Path $project 'build/startup-internal-003-comparison/candidate/lenteuv.exe'
$expectedHash = '874F5783B219E24FE1AD74B330BBF9A344EF2DAB0E3664738CED579CF5FF50D6'
$target = 'F:\Pictures\FotosTEMP'
$helper = Join-Path $PSScriptRoot 'startup_etw_collector.ps1'
$validated = Get-Content -LiteralPath "$state/preparation.json" -Raw | ConvertFrom-Json
$principal = [Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
if ($principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) { throw 'Only the ETW helper may be elevated.' }
if (!$ConfirmFirstLaunch) { throw 'First launch confirmation required.' }
$boot = (Get-CimInstance Win32_OperatingSystem).LastBootUpTime.ToUniversalTime()
$bootId = $boot.Ticks.ToString()
$guardPath = Join-Path $state "attempt-$bootId.json"
if (Test-Path -LiteralPath $guardPath) { throw 'Boot opportunity already consumed. Never retry.' }
if (@(Get-Process lenteuv,async_preview_tests,ipc_diagnostic_tests -ErrorAction SilentlyContinue).Count) { throw 'Product/test process already running.' }
if (!$validated.CollectorValidated -or (Get-FileHash -LiteralPath $helper).Hash -ne $validated.CollectorHash) { throw 'ETW helper does not match validated collector.' }
if (!(Test-Path -LiteralPath $target -PathType Container)) { throw 'Agreed media directory unavailable.' }
$directory = Split-Path $candidate -Parent
$inputs = @($candidate,"$directory/user_data/settings.json","$directory/user_data/session.txt")
# Metadata checks only: do not read candidate code pages or multimedia before capture.
$stamps = @($inputs | ForEach-Object {
    $item = Get-Item -LiteralPath $_
    if ($item.LastWriteTimeUtc -ge $boot) { throw 'Prepared input changed after boot; no launch allowed.' }
    [pscustomobject]@{Path=$item.FullName;Length=$item.Length;LastWriteUtcTicks=$item.LastWriteTimeUtc.Ticks}
})
if ($stamps[0].Length -ne 1064960) { throw 'Unexpected prepared executable length.' }
$run = Join-Path $project ('build/startup-internal-003-cold-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $run | Out-Null
# Local-only manifest writer; media content and filenames are never enumerated here.
function Save-Json([string]$Path, [object]$Value) {
    [IO.File]::WriteAllText($Path,($Value | ConvertTo-Json -Depth 8),[Text.UTF8Encoding]::new($false))
}
$instance = 'LenteUV003-'+[guid]::NewGuid().ToString('N')
Save-Json "$run/request.json" @{Instance=$instance;Candidate=$candidate;Target=$target;BootId=$bootId}
$record = [ordered]@{BootUtc=$boot.ToString('o');BootId=$bootId;Candidate=$candidate;ExpectedHash=$expectedHash;Target=$target
    InputsBefore=$stamps;Arguments=@('--multiple',$target);ProductLaunchAttempted=$false;ElevatedProduct=$false;Error=$null}
$collector=$null; $child=$null
try {
    $arguments=@('-NoProfile','-NonInteractive','-ExecutionPolicy','Bypass','-File',('"'+$helper+'"'),'-RunRoot',('"'+$run+'"'),'-Instance',$instance)
    $collector=Start-Process -FilePath "$env:SystemRoot/System32/WindowsPowerShell/v1.0/powershell.exe" -Verb RunAs -WindowStyle Hidden -ArgumentList $arguments -PassThru
    $deadline=[DateTime]::UtcNow.AddSeconds(60)
    while (!(Test-Path -LiteralPath "$run/ready.json")) {
        if ($collector.HasExited -or [DateTime]::UtcNow -gt $deadline) { throw 'ETW readiness failed; no product launch.' }
        Start-Sleep -Milliseconds 100
    }
    $ready=Get-Content -LiteralPath "$run/ready.json" -Raw | ConvertFrom-Json
    if ($ready.Instance -ne $instance -or !$ready.Administrator -or $ready.Status -ne 'recording') { throw 'Invalid ETW handshake.' }
    if (@(Get-Process lenteuv,async_preview_tests,ipc_diagnostic_tests -ErrorAction SilentlyContinue).Count) { throw 'Product/test appeared before capture.' }
    foreach ($stamp in $stamps) {
        $item=Get-Item -LiteralPath $stamp.Path
        if ($item.Length -ne $stamp.Length -or $item.LastWriteTimeUtc.Ticks -ne $stamp.LastWriteUtcTicks) { throw 'Prepared input changed during ETW setup.' }
    }
    $info=[Diagnostics.ProcessStartInfo]::new($candidate)
    $info.UseShellExecute=$false; $info.WorkingDirectory=$directory
    $info.ArgumentList.Add('--multiple'); $info.ArgumentList.Add($target)
    foreach ($key in @($info.Environment.Keys | Where-Object { $_ -like 'LENTEUV_PERF_*' -or $_ -eq 'LENTEUV_CACHE_NAMESPACE' })) { [void]$info.Environment.Remove($key) }
    $info.Environment['LENTEUV_PERF_OUTPUT']="$run/qpc.csv"
    $info.Environment['LENTEUV_PERF_SCENARIO']='viewer'
    $info.Environment['LENTEUV_PERF_EXIT_MS']='15000'
    $info.Environment['LENTEUV_PERF_READY_EXIT_MS']='2500'
    $guard=[IO.File]::Open($guardPath,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
    $guard.Dispose()
    Save-Json $guardPath @{RunRoot=$run;BootId=$bootId;NeverRetry=$true;Candidate=$candidate}
    $record.Frequency=[Diagnostics.Stopwatch]::Frequency
    $record.InvocationUtc=[DateTimeOffset]::UtcNow.ToString('o')
    $record.ProductLaunchAttempted=$true
    $record.InvocationQpc=[Diagnostics.Stopwatch]::GetTimestamp()
    $child=[Diagnostics.Process]::Start($info)
    $record.StartReturnedQpc=[Diagnostics.Stopwatch]::GetTimestamp()
    $record.ProductPid=$child.Id
    $record.CreationFileTime=$child.StartTime.ToUniversalTime().ToFileTimeUtc()
    Save-Json "$run/capture.json" $record
    if (!$child.WaitForExit(45000)) { [void]$child.CloseMainWindow(); throw 'Own product timeout; normal close requested. Never retry.' }
    $record.ExitCode=$child.ExitCode
} catch { $record.Error=$_.Exception.Message }
finally {
    [IO.File]::WriteAllText("$run/stop.request",'stop owned collector')
    if ($collector) { [void]$collector.WaitForExit(55000); $collector.Dispose() }
    if ($child) { $child.Dispose() }
    Save-Json "$run/capture.json" $record
}
if ($record.Error) { throw "$($record.Error) Artifacts: $run" }
$record.ExecutableHashAfter=(Get-FileHash -LiteralPath $candidate).Hash
$record.SettingsHashAfter=(Get-FileHash -LiteralPath $inputs[1]).Hash
$record.SessionHashAfter=(Get-FileHash -LiteralPath $inputs[2]).Hash
Save-Json "$run/capture.json" $record
if ($record.ExecutableHashAfter -ne $expectedHash -or $record.ExitCode -ne 0) { throw "Candidate verification failed. Never retry. Artifacts: $run" }
$result=Get-Content -LiteralPath "$run/collector-result.json" -Raw | ConvertFrom-Json
if ($result.Status -ne 'saved' -or !$result.EtlExists) { throw "ETW save failed. Never retry. Artifacts: $run" }
[pscustomobject]@{RunRoot=$run;ProductLaunchAttempted=$record.ProductLaunchAttempted;ExitCode=$record.ExitCode;EtlSaved=$true} | ConvertTo-Json
