# Re-enters Viewer/Waterfall in own isolated native processes; never moves the physical cursor.
param(
    [Parameter(Mandatory)][string]$ComparisonRoot,
    [Parameter(Mandatory)][string]$Images,
    [Parameter(Mandatory)][string]$RunRoot,
    [string]$CandidateComparisonRoot,
    [ValidateRange(2,120)][int]$PreparationSeconds=30,
    [ValidateSet('base','candidate')][string[]]$Variants=@('base','candidate'),
    [int[]]$Positions=@(),
    [ValidateRange(1,30)][int]$Repetitions=10
)
$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
$root=[IO.Path]::GetFullPath($RunRoot)
if (!$root.StartsWith("$project\build\",[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $root)) { throw 'RunRoot must be new and inside build.' }
New-Item -ItemType Directory -Path $root | Out-Null
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class ReuseBenchmarkNative {
    [StructLayout(LayoutKind.Sequential)] public struct Rect { public int Left,Top,Right,Bottom; }
    [DllImport("user32.dll")] public static extern bool PostMessageW(IntPtr window,uint message,IntPtr wparam,IntPtr lparam);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr window,out Rect rect);
}
'@
$rows=@()
foreach ($variant in $Variants) {
    $sourceRoot=if ($variant -eq 'candidate' -and $CandidateComparisonRoot) { $CandidateComparisonRoot } else { $ComparisonRoot }
    $source=Join-Path $sourceRoot "$variant-waterfall-warm"
    $manifest=Get-Content -LiteralPath "$source\manifest.json" -Raw | ConvertFrom-Json
    foreach ($image in $manifest.SourceFiles) {
        if ($Positions.Count -and $Positions -notcontains $image.Position) { continue }
        $run=Join-Path $root "$variant-$($image.Position)"
        Copy-Item -LiteralPath "$source\app" -Destination $run -Recurse
        $trace=Join-Path $run 'reuse-trace.csv'
        $start=[Diagnostics.ProcessStartInfo]::new("$run\lenteuv.exe")
        $start.UseShellExecute=$false; $start.WorkingDirectory=$run
        $start.ArgumentList.Add('--multiple'); $start.ArgumentList.Add((Join-Path $Images $image.Name))
        $start.Environment['LENTEUV_PERF_OUTPUT']=$trace
        $start.Environment['LENTEUV_PERF_SCENARIO']='waterfall'
        $start.Environment['LENTEUV_PERF_EXIT_MS']=([Math]::Max(60,$PreparationSeconds+40)*1000).ToString()
        $start.Environment['LENTEUV_PERF_READY_EXIT_MS']='0'
        $start.Environment['LENTEUV_CACHE_NAMESPACE']='warm'
        $child=[Diagnostics.Process]::Start($start)
        $demands=@()
        try {
            # Initial viewport is prepared from the already measured persistent cache, independently of reuse cycles.
            Start-Sleep -Seconds 4
            $child.Refresh(); $window=$child.MainWindowHandle
            if ($child.HasExited -or !$window) { throw 'Own native viewer did not create a window.' }
            $rect=[ReuseBenchmarkNative+Rect]::new()
            if (![ReuseBenchmarkNative]::GetClientRect($window,[ref]$rect)) { throw 'Own client geometry unavailable.' }
            $point=[IntPtr](($rect.Bottom-5 -shl 16) -bor 200)
            # Prepare the independent 100px strip variant and let its preserved scroll finish.
            # This is outside measured cycles; exported events later verify both viewports were ready.
            [void][ReuseBenchmarkNative]::PostMessageW($window,0x100,[IntPtr]0x61,[IntPtr]0)
            [void][ReuseBenchmarkNative]::PostMessageW($window,0x200,[IntPtr]0,$point)
            $remaining=$PreparationSeconds
            while ($remaining -gt 0) {
                $pause=[Math]::Min(30,$remaining)
                Start-Sleep -Seconds $pause
                $remaining-=$pause
                Write-Host "Preparation $variant/$($image.Position): $remaining seconds remaining"
            }
            [void][ReuseBenchmarkNative]::PostMessageW($window,0x200,[IntPtr]0,$point)
            Start-Sleep -Milliseconds 500
            [void][ReuseBenchmarkNative]::PostMessageW($window,0x100,[IntPtr]0x63,[IntPtr]0)
            Start-Sleep -Milliseconds 500
            for ($iteration=0;$iteration -lt $Repetitions;++$iteration) {
                $before=[Diagnostics.Stopwatch]::GetTimestamp()
                if (![ReuseBenchmarkNative]::PostMessageW($window,0x100,[IntPtr]0x61,[IntPtr]0)) { throw 'Viewer input failed.' }
                [void][ReuseBenchmarkNative]::PostMessageW($window,0x200,[IntPtr]0,$point)
                Start-Sleep -Milliseconds 350
                $next=[Diagnostics.Stopwatch]::GetTimestamp()
                $demands+=[pscustomobject]@{Scenario='strip';Iteration=$iteration;Begin=$before;End=$next}
                if (![ReuseBenchmarkNative]::PostMessageW($window,0x100,[IntPtr]0x63,[IntPtr]0)) { throw 'Waterfall input failed.' }
                Start-Sleep -Milliseconds 500
                $demands+=[pscustomobject]@{Scenario='waterfall';Iteration=$iteration;Begin=$next;End=[Diagnostics.Stopwatch]::GetTimestamp()}
            }
            [void][ReuseBenchmarkNative]::PostMessageW($window,0x10,[IntPtr]0,[IntPtr]0)
            if (!$child.WaitForExit(5000)) { throw 'Own viewer did not close.' }
            if ($child.ExitCode -ne 0) { throw "Own viewer exit $($child.ExitCode)" }
        } finally {
            if (!$child.HasExited) { $child.Kill(); $child.WaitForExit() }
            $child.Dispose()
        }
        $events=@(Import-Csv -LiteralPath $trace)
        $frequency=[double]$events[0].frequency
        $beforeCycles=@($events | Where-Object { [long]$_.qpc -lt $demands[0].Begin })
        $prepared=@($beforeCycles | Where-Object phase -eq 'strip.viewport.submitted').Count -gt 0 -and
            @($beforeCycles | Where-Object phase -eq 'waterfall.viewport.submitted').Count -gt 0
        foreach ($demand in $demands) {
            $slice=@($events | Where-Object { [long]$_.qpc -ge $demand.Begin -and [long]$_.qpc -lt $demand.End })
            $frame=$slice | Where-Object phase -eq "$($demand.Scenario).viewport.submitted" | Select-Object -First 1
            $rows+=[pscustomobject]@{
                Variant=$variant;Position=$image.Position;Iteration=$demand.Iteration;Scenario=$demand.Scenario
                RequestToSubmittedMs=if ($frame) { ([long]$frame.qpc-$demand.Begin)*1000/$frequency } else { $null }
                SourceDecodes=@($slice | Where-Object phase -eq 'decode.begin').Count
                CacheHits=@($slice | Where-Object phase -eq 'cache.hit').Count
                Failures=@($slice | Where-Object { $_.phase -in 'strip.failed','waterfall.failed' }).Count
                ExecutableHash=$manifest.ExecutableHash
                PreparedBeforeCycles=$prepared;PreparationSeconds=$PreparationSeconds
            }
        }
        $rows | Export-Csv -LiteralPath "$root\samples.csv" -NoTypeInformation
        Write-Host "Reuse completed: $variant position $($image.Position)"
    }
}
