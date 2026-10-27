# Compares native first-frame/navigation latency sequentially on copied fixtures and executable-local data.
param(
    [Parameter(Mandatory)][string]$ReferenceExecutable,
    [Parameter(Mandatory)][string]$CandidateExecutable,
    [Parameter(Mandatory)][string]$RunRoot,
    [ValidateRange(1,100)][int]$Repetitions=30,
    [ValidateRange(1,10000)][int]$FileCount=1000
)
$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
$root=[IO.Path]::GetFullPath($RunRoot)
if (!$root.StartsWith("$project\build\",[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $root)) { throw 'RunRoot must be new and inside build.' }
New-Item -ItemType Directory -Path "$root\images" -Force | Out-Null
for ($i=0;$i -lt $FileCount;$i++) {
    $png=($i%10 -eq 0)
    $source=if ($png) { "$project\tests\fixtures\linux.png" } else { "$project\tests\fixtures\noname.jpg" }
    $ext=if ($png) { 'png' } else { 'jpg' }
    Copy-Item -LiteralPath $source -Destination (Join-Path "$root\images" ('image-{0:D5}.{1}' -f $i,$ext))
}
$original=Join-Path "$root\images" 'image-00001.jpg'
$variants=@{base=(Resolve-Path -LiteralPath $ReferenceExecutable).Path; candidate=(Resolve-Path -LiteralPath $CandidateExecutable).Path}
$manifest=[ordered]@{CreatedUtc=[DateTime]::UtcNow.ToString('o');Repetitions=$Repetitions;FileCount=$FileCount;OS=[Environment]::OSVersion.VersionString;Processor=$env:PROCESSOR_IDENTIFIER;SourceRevision=(& git -C $project rev-parse HEAD);Binaries=@{};Dataset='Copied JPEG/PNG fixtures: 90% JPEG, 10% PNG; application cache warm, OS cache uncontrolled'}
foreach ($variant in @('base','candidate')) {
    New-Item -ItemType Directory -Path "$root\$variant\user_data" -Force | Out-Null
    Copy-Item -LiteralPath $variants[$variant] -Destination "$root\$variant\image_viewer.exe"
    Get-ChildItem -LiteralPath (Split-Path $variants[$variant]) -Filter *.dll -File | Copy-Item -Destination "$root\$variant"
    Copy-Item -LiteralPath "$project\locales" -Destination "$root\$variant\locales" -Recurse
    $manifest.Binaries[$variant]=@{Source=$variants[$variant];SHA256=(Get-FileHash -LiteralPath $variants[$variant]).Hash}
}
$manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath "$root\manifest.json" -Encoding utf8NoBOM
$samples=@()
foreach ($scenario in @('defaults','saved')) {
    foreach ($variant in @('base','candidate')) {
        $settings=Get-Content -LiteralPath "$project\packaging\default-settings.json" -Raw | ConvertFrom-Json
        $settings.settings.single_instance[0]=$false
        $settings.settings.hide_to_tray_on_close[0]=$false
        $settings.settings.run_on_windows_startup[0]=$false
        $settings.settings.sort_mode[0]=if ($scenario -eq 'saved') { 5 } else { 0 }
        $settings.settings.image_filter_mask[0]=if ($scenario -eq 'saved') { 2 } else { 511 }
        $settings | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath "$root\$variant\user_data\settings.json" -Encoding utf8NoBOM
    }
    for ($iteration=-2;$iteration -lt $Repetitions;$iteration++) {
        $order=if ($iteration%2) { @('candidate','base') } else { @('base','candidate') }
        foreach ($variant in $order) {
            $stage="$root\$variant"
            $trace="$root\$scenario-$variant-$iteration.csv"
            $start=[Diagnostics.ProcessStartInfo]::new("$stage\image_viewer.exe")
            $start.UseShellExecute=$false; $start.WorkingDirectory=$stage; $start.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
            $start.ArgumentList.Add('--multiple'); $start.ArgumentList.Add($original)
            $start.Environment['IMAGE_VIEWER_PERF_OUTPUT']=$trace
            $start.Environment['IMAGE_VIEWER_PERF_EXIT_MS']='10000'
            $start.Environment['IMAGE_VIEWER_PERF_READY_EXIT_MS']='250'
            $start.Environment['IMAGE_VIEWER_PERF_SCENARIO']='viewer'
            $invocation=[Diagnostics.Stopwatch]::GetTimestamp()
            $child=[Diagnostics.Process]::Start($start)
            try {
                if (!$child.WaitForExit(20000)) { throw "Native process did not exit within diagnostic budget: $($child.Id)" }
                if ($child.ExitCode) { throw "Native process failed: $($child.ExitCode)" }
            } finally { $child.Dispose() }
            $events=@(Import-Csv -LiteralPath $trace)
            $entry=$events | Where-Object phase -eq 'main.entry' | Select-Object -First 1
            $frame=$events | Where-Object phase -eq 'frame.original.submitted' | Select-Object -First 1
            $navigation=$events | Where-Object phase -eq 'navigation.ready' | Select-Object -First 1
            if (!$entry -or !$frame -or !$navigation) { throw "Incomplete native startup trace: $trace" }
            if (@($events | Where-Object { $_.phase -eq 'trace.dropped' -and [long]$_.value -ne 0 }).Count) { throw 'Trace overflow.' }
            $row=[pscustomobject]@{Scenario=$scenario;Variant=$variant;Iteration=$iteration;Warmup=($iteration -lt 0);InvocationToSubmittedMs=([long]$frame.qpc-$invocation)*1000/[double]$entry.frequency;EntryToSubmittedMs=([long]$frame.qpc-[long]$entry.qpc)*1000/[double]$entry.frequency;NavigationMs=([long]$navigation.qpc-$invocation)*1000/[double]$entry.frequency}
            $samples+=$row
            $samples | Export-Csv -LiteralPath "$root\samples.csv" -NoTypeInformation
        }
        if ($iteration%10 -eq 9) { Write-Host "${scenario}: $($iteration+1)/$Repetitions paired trials" }
    }
}
$summary=@()
foreach ($scenario in @('defaults','saved')) {
    foreach ($metric in @('InvocationToSubmittedMs','EntryToSubmittedMs','NavigationMs')) {
        $values=@{}
        foreach ($variant in @('base','candidate')) {
            $ordered=@($samples | Where-Object { $_.Scenario -eq $scenario -and $_.Variant -eq $variant -and !$_.Warmup } | ForEach-Object { $_.$metric } | Sort-Object)
            $values[$variant]=@{Median=($ordered[[int][Math]::Floor(($ordered.Count-1)/2)]+$ordered[[int][Math]::Ceiling(($ordered.Count-1)/2)])/2;P95=$ordered[[int][Math]::Ceiling($ordered.Count*0.95)-1]}
        }
        $medianDelta=100*($values.candidate.Median/$values.base.Median-1)
        $p95Delta=100*($values.candidate.P95/$values.base.P95-1)
        $summary+=[pscustomobject]@{Scenario=$scenario;Metric=$metric;BaseMedian=$values.base.Median;CandidateMedian=$values.candidate.Median;MedianChangePercent=$medianDelta;BaseP95=$values.base.P95;CandidateP95=$values.candidate.P95;P95ChangePercent=$p95Delta;WithinBudget=($medianDelta -le 5 -and $p95Delta -le 10)}
    }
}
$summary | Export-Csv -LiteralPath "$root\summary.csv" -NoTypeInformation
$summary | Format-Table -AutoSize
