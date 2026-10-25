# Paired subsequent launches against a read-only folder; all product writes use fresh build staging.
param(
    [Parameter(Mandatory)][string]$Baseline,
    [Parameter(Mandatory)][string]$Candidate,
    [Parameter(Mandatory)][string]$Images,
    [Parameter(Mandatory)][string]$RunRoot,
    [ValidateRange(2,30)][int]$Pairs = 8
)
$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
$run = [IO.Path]::GetFullPath($RunRoot)
if (!$run.StartsWith("$project\build\", [StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $run)) {
    throw 'RunRoot must be a new directory inside project/build.'
}
$imagesPath = (Resolve-Path -LiteralPath $Images).Path
if (!(Test-Path -LiteralPath $imagesPath -PathType Container)) { throw 'Images must be a directory.' }
$executables = @{ baseline = (Resolve-Path -LiteralPath $Baseline).Path; candidate = (Resolve-Path -LiteralPath $Candidate).Path }
$settings = Get-Content -LiteralPath "$project\packaging\default-settings.json" -Raw | ConvertFrom-Json
foreach ($name in @('single_instance','hide_to_tray_on_close','hide_on_app_start','enable_file_log','enable_debug_ipc')) {
    if ($settings.settings.PSObject.Properties.Name -contains $name) { $settings.settings.$name[0] = $false }
}
$settings.settings.show_fullscreen[0] = $true
foreach ($variant in @('baseline','candidate')) {
    $stage = Join-Path $run $variant
    New-Item -ItemType Directory -Path "$stage\user_data" | Out-Null
    Copy-Item -LiteralPath $executables[$variant] -Destination "$stage\lenteuv.exe"
    $runtime = @(Get-ChildItem -LiteralPath (Split-Path $executables[$variant]) -Filter *.dll -File)
    if (!$runtime.Count) { $runtime = @(Get-ChildItem -LiteralPath (Split-Path $executables.baseline) -Filter *.dll -File) }
    $runtime | Copy-Item -Destination $stage
    Copy-Item -LiteralPath "$project\locales" -Destination "$stage\locales" -Recurse
    $settings | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath "$stage\user_data\settings.json" -Encoding utf8NoBOM
    [IO.File]::WriteAllText("$stage\user_data\session.txt", "$imagesPath`n`n`n`n", [Text.UTF8Encoding]::new($false))
}
$rows = @()
# First pair is retained separately as conditioning, never labeled as a first launch after reboot.
for ($pair = 0; $pair -le $Pairs; ++$pair) {
    $order = if ($pair % 2) { @('candidate','baseline') } else { @('baseline','candidate') }
    foreach ($variant in $order) {
        $stage = Join-Path $run $variant
        $trace = Join-Path $run "$variant-$pair.csv"
        $start = [Diagnostics.ProcessStartInfo]::new("$stage\lenteuv.exe")
        $start.UseShellExecute = $false
        $start.WorkingDirectory = $stage
        $start.ArgumentList.Add('--multiple')
        $start.ArgumentList.Add($imagesPath)
        $start.Environment['LENTEUV_PERF_OUTPUT'] = $trace
        $start.Environment['LENTEUV_PERF_EXIT_MS'] = '10000'
        $start.Environment['LENTEUV_PERF_READY_EXIT_MS'] = '250'
        $start.Environment['LENTEUV_PERF_SCENARIO'] = 'viewer'
        $invoke = [Diagnostics.Stopwatch]::GetTimestamp()
        $child = [Diagnostics.Process]::Start($start)
        try {
            if (!$child.WaitForExit(20000)) { $child.Kill(); throw 'Own benchmark process timed out.' }
            if ($child.ExitCode -ne 0) { throw "Product exited $($child.ExitCode)" }
        } finally { $child.Dispose() }
        $events = @(Import-Csv -LiteralPath $trace)
        $entry = $events | Where-Object phase -eq 'main.entry' | Select-Object -First 1
        $original = $events | Where-Object phase -eq 'frame.original.submitted' | Select-Object -First 1
        if (!$original -or !$entry) { throw 'Missing original presentation/entry event.' }
        $frequency = [double]$entry.frequency
        $row = [ordered]@{ variant=$variant; pair=$pair; conditioning=($pair -eq 0)
            entryToOriginalMs=([double]$original.qpc-[double]$entry.qpc)*1000/$frequency
            invokeToOriginalMs=([double]$original.qpc-$invoke)*1000/$frequency
            enumerationCount=@($events | Where-Object phase -eq 'folder.enumerate.begin').Count }
        foreach ($phase in @('renderer.initialize','renderer.d3d','renderer.d2d','renderer.dwrite','renderer.swapchain','renderer.composition','renderer.target','folder.enumerate','folder.order','viewer.decode','upload')) {
            $begin = $events | Where-Object phase -eq "$phase.begin" | Select-Object -First 1
            $end = $events | Where-Object phase -eq "$phase.end" | Select-Object -First 1
            if ($begin -and $end) { $row[$phase+'Ms'] = ([double]$end.qpc-[double]$begin.qpc)*1000/$frequency }
        }
        $rows += [pscustomobject]$row
    }
}
$rows | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath "$run\results.json" -Encoding utf8NoBOM
foreach ($variant in @('baseline','candidate')) {
    $values = @($rows | Where-Object { $_.variant -eq $variant -and !$_.conditioning } | Sort-Object entryToOriginalMs)
    $median = ($values[[int][Math]::Floor(($values.Count-1)/2)].entryToOriginalMs + $values[[int][Math]::Ceiling(($values.Count-1)/2)].entryToOriginalMs)/2
    [pscustomobject]@{ variant=$variant; pairs=$values.Count; medianEntryToOriginalMs=[Math]::Round($median,2)
        minMs=[Math]::Round($values[0].entryToOriginalMs,2); maxMs=[Math]::Round($values[-1].entryToOriginalMs,2)
        sha256=(Get-FileHash -LiteralPath $executables[$variant] -Algorithm SHA256).Hash }
}
