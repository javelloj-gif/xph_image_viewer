# Measures real product processes in an isolated writable portable tree; originals are read-only inputs.
param(
    [Parameter(Mandatory)][string]$Executable,
    [Parameter(Mandatory)][string]$Images,
    [Parameter(Mandatory)][string]$RunRoot,
    [ValidateRange(1,100)][int]$Repetitions = 30,
    [ValidateSet('viewer','strip','waterfall')][string]$Scenario = 'viewer',
    [ValidateSet('warm','empty')][string]$Cache = 'warm',
    [switch]$Resources,
    [ValidateRange(0,3)][int]$Warmups = 1,
    [switch]$Windowed,
    [string]$SourceRevision
)
$ErrorActionPreference = 'Stop'
# Reads JPEG frame dimensions from segment headers only; never decodes pixels or writes source metadata.
function Get-JpegDimensions([string]$Path) {
    $stream=[IO.File]::Open($Path,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
    $reader=[IO.BinaryReader]::new($stream)
    try {
        if ($reader.ReadByte() -ne 255 -or $reader.ReadByte() -ne 216) { return $null }
        while ($stream.Position -lt $stream.Length) {
            if ($reader.ReadByte() -ne 255) { continue }
            do { $marker=[int]$reader.ReadByte() } while ($marker -eq 255)
            if ($marker -eq 217 -or $marker -eq 218) { return $null }
            if ($marker -eq 0 -or $marker -eq 216 -or ($marker -ge 208 -and $marker -le 215)) { continue }
            $length=([int]$reader.ReadByte() -shl 8) -bor [int]$reader.ReadByte()
            if ($length -lt 2) { return $null }
            if ($marker -in 192,193,194,195,197,198,199,201,202,203,205,206,207) {
                [void]$reader.ReadByte()
                $height=([int]$reader.ReadByte() -shl 8) -bor [int]$reader.ReadByte()
                $width=([int]$reader.ReadByte() -shl 8) -bor [int]$reader.ReadByte()
                return [pscustomobject]@{Width=$width;Height=$height}
            }
            [void]$stream.Seek($length-2,[IO.SeekOrigin]::Current)
        }
    } catch { return $null } finally { $reader.Dispose() }
}
if (!('StartupBenchmarkNative' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class StartupBenchmarkNative {
    [DllImport("kernel32.dll")] public static extern void GetSystemTimePreciseAsFileTime(out long value);
    [StructLayout(LayoutKind.Sequential)] public struct IO { public ulong ReadOps,WriteOps,OtherOps,ReadBytes,WriteBytes,OtherBytes; }
    [DllImport("kernel32.dll")] public static extern bool GetProcessIoCounters(IntPtr process,out IO io);
    [StructLayout(LayoutKind.Sequential)] public struct Memory { public uint Length,Load; public ulong TotalPhysical,AvailablePhysical,TotalPage,AvailablePage,TotalVirtual,AvailableVirtual,AvailableExtended; }
    [DllImport("kernel32.dll")] public static extern bool GlobalMemoryStatusEx(ref Memory value);
}
'@
}
$project = Split-Path $PSScriptRoot -Parent
if (!$SourceRevision) { $SourceRevision=(& git -C $project rev-parse HEAD) }
$exe = (Resolve-Path -LiteralPath $Executable).Path
$imagesPath = (Resolve-Path -LiteralPath $Images).Path
$run = [IO.Path]::GetFullPath($RunRoot)
if (Test-Path -LiteralPath $run) { throw 'RunRoot must be a new directory.' }
if (!$run.StartsWith("$project\build\", [StringComparison]::OrdinalIgnoreCase)) { throw 'RunRoot must be inside project/build.' }
New-Item -ItemType Directory -Path $run | Out-Null
$stage = Join-Path $run 'app'
New-Item -ItemType Directory -Path "$stage\user_data" | Out-Null
Copy-Item -LiteralPath $exe -Destination "$stage\lenteuv.exe"
$runtime = @(Get-ChildItem -LiteralPath (Split-Path $exe) -Filter *.dll -File)
if (!$runtime.Count) { $runtime = @(Get-ChildItem -LiteralPath "$project\dist\lenteuv_qa_002_20261023_sin_library" -Filter *.dll -File) }
$runtime | Copy-Item -Destination $stage
Copy-Item -LiteralPath "$project\locales" -Destination "$stage\locales" -Recurse
$settings = Get-Content -LiteralPath "$project\packaging\default-settings.json" -Raw | ConvertFrom-Json
foreach ($setting in @('single_instance','hide_to_tray_on_close','hide_on_app_start','enable_file_log','enable_debug_ipc')) {
    if ($settings.settings.PSObject.Properties.Name -contains $setting) { $settings.settings.$setting[0]=$false }
}
if ($Windowed) { $settings.settings.show_fullscreen[0]=$false }
$settings | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath "$stage\user_data\settings.json" -Encoding utf8NoBOM
[IO.File]::WriteAllText("$stage\user_data\session.txt", "$imagesPath`n`n`n`n", [Text.UTF8Encoding]::new($false))
$files = @(Get-ChildItem -LiteralPath $imagesPath -File | Where-Object { $_.Extension -in '.jpg','.jpeg' })
[Array]::Sort($files,[Comparison[IO.FileInfo]]{param($left,$right) [StringComparer]::OrdinalIgnoreCase.Compare($left.FullName,$right.FullName)})
if ($files.Count -lt 3) { throw 'Need at least three JPEG files.' }
$referencePixels=0L
$positions = foreach ($target in @(0, ([int][Math]::Floor($files.Count/2)), ($files.Count-1))) {
    $eligible=@(0..($files.Count-1) | Where-Object { $files[$_].Length -ge 4250000 -and $files[$_].Length -le 5750000 })
    if (!$eligible.Count) { throw 'No comparable JPEGs in the agreed approximately 5 MB size range.' }
    $selected=$null
    foreach ($index in ($eligible | Sort-Object {[Math]::Abs($_-$target)})) {
        $dimensions=Get-JpegDimensions $files[$index].FullName
        if ($dimensions) {
            $pixels=[long]$dimensions.Width*$dimensions.Height
            if (!$referencePixels) { $referencePixels=$pixels }
            if ($pixels -ge $referencePixels*0.9 -and $pixels -le $referencePixels*1.1) { $selected=$index; break }
        }
    }
    if ($null -eq $selected) { throw 'No comparable JPEG resolution in the agreed byte range.' }
    $selected
}
$rows = @()
foreach ($position in $positions) {
    $file = $files[$position]
    $dimensions=Get-JpegDimensions $file.FullName
    for ($iteration=-$Warmups; $iteration -lt $Repetitions; ++$iteration) {
        # Empty mode uses a unique disposable cache namespace, never deletes a user's cache.
        $trace = Join-Path $run "trace-$position-$iteration.csv"
        $start = [Diagnostics.ProcessStartInfo]::new("$stage\lenteuv.exe")
        $start.UseShellExecute = $false
        $start.WorkingDirectory = $stage
        $start.ArgumentList.Add('--multiple')
        $start.ArgumentList.Add($file.FullName)
        $start.Environment['LENTEUV_PERF_OUTPUT'] = $trace
        $start.Environment['LENTEUV_PERF_EXIT_MS'] = if ($Resources) { '15000' } else { '10000' }
        $start.Environment['LENTEUV_PERF_READY_EXIT_MS'] = if ($Resources) { '0' } else { '250' }
        if ($iteration -lt 0 -and !$Resources -and $Scenario -eq 'viewer') {
            $start.Environment['LENTEUV_PERF_EXIT_MS']='3000'
            $start.Environment['LENTEUV_PERF_READY_EXIT_MS']='0'
        }
        $start.Environment['LENTEUV_PERF_SCENARIO'] = $Scenario
        $start.Environment['LENTEUV_CACHE_NAMESPACE'] = if ($Cache -eq 'empty') { "empty-$position-$iteration" } else { 'warm' }
        $syncBefore=[Diagnostics.Stopwatch]::GetTimestamp()
        $syncFileTime=0L
        [StartupBenchmarkNative]::GetSystemTimePreciseAsFileTime([ref]$syncFileTime)
        $syncAfter=[Diagnostics.Stopwatch]::GetTimestamp()
        $syncQpc=($syncBefore+$syncAfter)/2.0
        $invocation = [Diagnostics.Stopwatch]::GetTimestamp()
        $child = [Diagnostics.Process]::Start($start)
        $creationFileTime=$child.StartTime.ToUniversalTime().ToFileTimeUtc()
        $peak = 0L
        $cpuMs = 0.0
        $io=[StartupBenchmarkNative+IO]::new()
        $gpuRows=@()
        $resourceNext=[DateTime]::UtcNow.AddSeconds(3)
        try {
            $deadline = [DateTime]::UtcNow.AddSeconds(30)
            while (!$child.WaitForExit(25)) {
                if ([DateTime]::UtcNow -gt $deadline) { $child.Kill(); throw 'Own benchmark process timed out.' }
                $child.Refresh()
                try { $peak=[Math]::Max($peak,$child.PeakWorkingSet64); $cpuMs=$child.TotalProcessorTime.TotalMilliseconds } catch { }
                [void][StartupBenchmarkNative]::GetProcessIoCounters($child.Handle,[ref]$io)
                if ($Resources -and [DateTime]::UtcNow -ge $resourceNext) {
                    $resourceNext=[DateTime]::UtcNow.AddSeconds(1)
                    try {
                        $counters=Get-Counter -Counter '\GPU Engine(*)\Utilization Percentage','\GPU Process Memory(*)\Dedicated Usage','\GPU Process Memory(*)\Shared Usage' -ErrorAction Stop
                        $gpuRows+=@($counters.CounterSamples | Where-Object { $_.InstanceName -match "pid_$($child.Id)_" } | Select-Object Timestamp,InstanceName,Path,Status,CookedValue)
                    } catch { $gpuRows+=[pscustomobject]@{Timestamp=[DateTime]::UtcNow;InstanceName='unavailable';Path='';Status=$_.Exception.Message;CookedValue=$null} }
                }
            }
            if ($child.ExitCode -ne 0) { throw "Product exited with $($child.ExitCode)" }
        } finally { $child.Dispose() }
        if (!(Test-Path -LiteralPath $trace)) { throw "Missing trace: $trace" }
        $events = @(Import-Csv -LiteralPath $trace)
        $entry = $events | Where-Object phase -eq 'main.entry' | Select-Object -First 1
        $frame = $events | Where-Object phase -eq 'frame.original.submitted' | Select-Object -First 1
        if (!$entry -or !$frame) { throw 'Missing original-frame marker.' }
        $frequency = [double]$entry.frequency
        $confirmed=$events | Where-Object phase -eq 'frame.original.display_confirmed' | Select-Object -First 1
        $navigation=$events | Where-Object phase -eq 'navigation.ready' | Select-Object -First 1
        $strip=$events | Where-Object phase -eq 'strip.target.prepared' | Select-Object -First 1
        $stripVisible=$events | Where-Object phase -eq 'strip.viewport.submitted' | Select-Object -First 1
        $waterfall=$events | Where-Object phase -eq 'waterfall.viewport.submitted' | Select-Object -First 1
        $demand=$events | Where-Object phase -eq 'waterfall.demand' | Select-Object -First 1
        $row = [pscustomobject]@{
            Position=$position; Iteration=$iteration; File=$file.Name; Bytes=$file.Length
            Width=$dimensions.Width; Height=$dimensions.Height
            Warmup=($iteration -lt 0)
            InvocationToSubmittedMs=([long]$frame.qpc-$invocation)*1000/$frequency
            EntryToSubmittedMs=([long]$frame.qpc-[long]$entry.qpc)*1000/$frequency
            ProcessCreationToSubmittedMs=([long]$frame.qpc-$syncQpc)*1000/$frequency-($creationFileTime-$syncFileTime)/10000.0
            ClockCorrelationUncertaintyUs=($syncAfter-$syncBefore)*1000000/$frequency
            InvocationToDisplayConfirmationMs=if ($confirmed) { ([long]$confirmed.qpc-$invocation)*1000/$frequency } else { $null }
            NavigationFromInvocationMs=if ($navigation) { ([long]$navigation.qpc-$invocation)*1000/$frequency } else { $null }
            StripPreparedAfterOriginalMs=if ($strip) { ([long]$strip.qpc-[long]$frame.qpc)*1000/$frequency } else { $null }
            StripVisibleAfterOriginalMs=if ($stripVisible) { ([long]$stripVisible.qpc-[long]$frame.qpc)*1000/$frequency } else { $null }
            WaterfallViewportMs=if ($waterfall -and $demand) { ([long]$waterfall.qpc-[long]$demand.qpc)*1000/$frequency } else { $null }
            CacheHits=@($events | Where-Object phase -eq 'cache.hit').Count
            SourceDecodes=@($events | Where-Object phase -eq 'decode.begin').Count
            ThumbnailFailures=@($events | Where-Object { $_.phase -in 'strip.failed','waterfall.failed' }).Count
            LogicalReadBytes=$io.ReadBytes; LogicalWriteBytes=$io.WriteBytes
            CacheDiskBytes=(Get-ChildItem -LiteralPath "$stage\user_data\cache" -Recurse -File -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum
            PeakWorkingSet=$peak; SampledCpuMs=$cpuMs
            Scenario=$Scenario; Cache=$Cache; Presentation=if ($confirmed) { 'DXGI-exact-present-id-observed-upper-bound' } else { 'submitted-only' }
        }
        if ($Resources) { $gpuRows | Export-Csv -LiteralPath "$run\gpu-$position-$iteration.csv" -NoTypeInformation }
        $rows += $row
        $rows | Export-Csv -LiteralPath "$run\samples.csv" -NoTypeInformation
    }
}
$summary = foreach ($group in ($rows | Where-Object { !$_.Warmup } | Group-Object Position)) {
    $values = @($group.Group.InvocationToSubmittedMs | Sort-Object)
    $mid=[int][Math]::Floor($values.Count/2)
    $median=if ($values.Count%2) { $values[$mid] } else { ($values[$mid-1]+$values[$mid])/2 }
    [pscustomobject]@{ Position=$group.Name; N=$values.Count; MedianMs=$median; P95Ms=$values[[int][Math]::Ceiling($values.Count*0.95)-1]; MaxMs=$values[-1] }
}
$manifest = [pscustomobject]@{
    ExecutableHash=(Get-FileHash -LiteralPath "$stage\lenteuv.exe").Hash
    Revision=$SourceRevision; RepositoryHead=(& git -C $project rev-parse HEAD); Dirty=(@(& git -C $project status --porcelain).Count -gt 0)
    Windows=[Environment]::OSVersion.VersionString; LogicalProcessors=[Environment]::ProcessorCount
    Fullscreen=$settings.settings.show_fullscreen[0]
    Processor=$env:PROCESSOR_IDENTIFIER
    SourceFiles=@($positions | ForEach-Object { [pscustomobject]@{Position=$_;Name=$files[$_].Name;Bytes=$files[$_].Length;Sha256=(Get-FileHash -LiteralPath $files[$_].FullName).Hash} })
    SettingsHash=(Get-FileHash -LiteralPath "$stage\user_data\settings.json").Hash
    Scenario=$Scenario; Cache=$Cache; Resources=[bool]$Resources; JpegCount=$files.Count; WarmupsPerPosition=$Warmups
    Note='Subsequent-session runs, not cold OS disk. QPC first original Present submission does not certify visibility or 100ms.'
    Summary=@($summary)
}
$memory=[StartupBenchmarkNative+Memory]::new(); $memory.Length=[Runtime.InteropServices.Marshal]::SizeOf($memory)
if ([StartupBenchmarkNative]::GlobalMemoryStatusEx([ref]$memory)) { $manifest | Add-Member -NotePropertyName PhysicalMemoryBytes -NotePropertyValue $memory.TotalPhysical }
$manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath "$run\manifest.json" -Encoding utf8
$summary | Format-Table
