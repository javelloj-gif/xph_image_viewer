# PowerShell 7 native diagnostic: copied product, generated image and private data; no user instance/data.
param(
    [Parameter(Mandatory)][string]$Executable,
    [Parameter(Mandatory)][string]$RunRoot,
    [ValidateRange(3,60)][int]$Seconds = 8,
    [ValidateRange(1,5)][int]$Repetitions = 3
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$sourceExe = (Resolve-Path -LiteralPath $Executable).Path
$runDirectory = [IO.Path]::GetFullPath((Join-Path $RunRoot ('gpu-idle-' + [guid]::NewGuid().ToString('N'))))
New-Item -ItemType Directory -Path $runDirectory | Out-Null
Copy-Item -LiteralPath $sourceExe -Destination (Join-Path $runDirectory 'image_viewer.exe')
Get-ChildItem -LiteralPath (Split-Path $sourceExe) -Filter '*.dll' | Copy-Item -Destination $runDirectory
Copy-Item -LiteralPath (Join-Path $projectRoot 'locales') -Destination $runDirectory -Recurse
New-Item -ItemType Directory -Path (Join-Path $runDirectory 'user_data'), (Join-Path $runDirectory 'test') | Out-Null
$settings = Get-Content -LiteralPath (Join-Path $projectRoot 'packaging/default-settings.json') -Raw | ConvertFrom-Json
$settings.settings.show_fullscreen[0] = $false
$settings.settings.single_instance[0] = $false
$settings.settings.hide_to_tray_on_close[0] = $false
$settings.settings.hide_on_app_start[0] = $false
$settings | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath (Join-Path $runDirectory 'user_data/settings.json') -Encoding utf8
Add-Type -AssemblyName System.Drawing
$seed = [Drawing.Bitmap]::new(2560,1440)
$graphics = [Drawing.Graphics]::FromImage($seed)
$brush = [Drawing.Drawing2D.LinearGradientBrush]::new([Drawing.Point]::new(0,0),[Drawing.Point]::new(2560,1440),[Drawing.Color]::DarkSlateBlue,[Drawing.Color]::Orange)
$graphics.FillRectangle($brush,0,0,2560,1440)
$seed.Save((Join-Path $runDirectory 'test/seed.png'),[Drawing.Imaging.ImageFormat]::Png)
$brush.Dispose(); $graphics.Dispose(); $seed.Dispose()
if (!('GpuIdleNative' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
// Own-child HWND discovery and controlled visibility/input for this diagnostic only.
public static class GpuIdleNative {
    private delegate bool EnumCallback(IntPtr hwnd,IntPtr data);
    [DllImport("user32.dll")] private static extern bool EnumWindows(EnumCallback callback,IntPtr data);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] private static extern int GetClassName(IntPtr hwnd,System.Text.StringBuilder text,int count);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] private static extern int GetWindowText(IntPtr hwnd,System.Text.StringBuilder text,int count);
    // Native query/input family: callers verify the child PID before operating its HWND.
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hwnd,int command);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr hwnd,IntPtr after,int x,int y,int w,int h,uint flags);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hwnd,uint msg,IntPtr wp,IntPtr lp);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hwnd,out uint pid);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr hwnd);
    // Finds the product class even when STARTUPINFO caused it to start hidden.
    public static IntPtr FindOwnedWindow(uint pid) {
        IntPtr result=IntPtr.Zero;
        EnumWindows((hwnd,data)=> {
            uint owner; GetWindowThreadProcessId(hwnd,out owner);
            if(owner!=pid) return true;
            var name=new System.Text.StringBuilder(256); GetClassName(hwnd,name,name.Capacity);
            if(name.ToString()=="LenteUVWindow") { result=hwnd; return false; }
            return true;
        },IntPtr.Zero);
        return result;
    }
    // Confirms the real image callback completed before the controller resizes the window.
    public static bool HasSeedTitle(IntPtr hwnd) {
        var text=new System.Text.StringBuilder(1024); GetWindowText(hwnd,text,text.Capacity);
        return text.ToString().Contains("seed.png");
    }
}
'@
}
$savedOutput = $env:IMAGE_VIEWER_PERF_OUTPUT
$savedExit = $env:IMAGE_VIEWER_PERF_EXIT_MS
$savedReadyExit = $env:IMAGE_VIEWER_PERF_READY_EXIT_MS
$savedScenario = $env:IMAGE_VIEWER_PERF_SCENARIO
$records = [Collections.Generic.List[object]]::new()
$stages = [Collections.Generic.List[object]]::new()
$process = $null
$completed = $false
try {
    $env:IMAGE_VIEWER_PERF_EXIT_MS = '0'
    $env:IMAGE_VIEWER_PERF_READY_EXIT_MS = '0'
    $env:IMAGE_VIEWER_PERF_SCENARIO = 'gpu-idle'
    for ($repetition=1; $repetition -le $Repetitions; $repetition++) {
        $tracePath = Join-Path $runDirectory "trace-$repetition.csv"
        $env:IMAGE_VIEWER_PERF_OUTPUT = $tracePath
        $startInfo = [Diagnostics.ProcessStartInfo]::new((Join-Path $runDirectory 'image_viewer.exe'))
        $startInfo.UseShellExecute = $false
        $startInfo.WindowStyle = [Diagnostics.ProcessWindowStyle]::Hidden
        $startInfo.WorkingDirectory = $runDirectory
        $startInfo.ArgumentList.Add('--multiple')
        $startInfo.ArgumentList.Add('--windowed')
        $startInfo.ArgumentList.Add((Join-Path $runDirectory 'test/seed.png'))
        $startInfo.Environment['IMAGE_VIEWER_PERF_OUTPUT'] = $tracePath
        $startInfo.Environment['IMAGE_VIEWER_PERF_EXIT_MS'] = '300000'
        $startInfo.Environment['IMAGE_VIEWER_PERF_READY_EXIT_MS'] = '0'
        $startInfo.Environment['IMAGE_VIEWER_PERF_SCENARIO'] = 'gpu-idle'
        $process = [Diagnostics.Process]::Start($startInfo)
        $deadline = [DateTime]::UtcNow.AddSeconds(15)
        do {
            Start-Sleep -Milliseconds 100
            $process.Refresh()
            if ($process.HasExited) { throw 'Product exited before window creation' }
            $hwnd = [GpuIdleNative]::FindOwnedWindow([uint32]$process.Id)
        } while ($hwnd -eq [IntPtr]::Zero -and [DateTime]::UtcNow -lt $deadline)
        if ($hwnd -eq [IntPtr]::Zero) { throw 'No product window within 15 seconds' }
        $windowOwner = [uint32]0
        $windowThread = [GpuIdleNative]::GetWindowThreadProcessId($hwnd,[ref]$windowOwner)
        if ($windowOwner -ne $process.Id) { throw "Window does not belong to measured child: HWND=$hwnd, child=$($process.Id), owner=$windowOwner, thread=$windowThread" }
        # Wait for production's image callback before resizing; it also confirms renderer/page initialization.
        while (![GpuIdleNative]::HasSeedTitle($hwnd) -and [DateTime]::UtcNow -lt $deadline) { Start-Sleep -Milliseconds 100 }
        if (![GpuIdleNative]::HasSeedTitle($hwnd)) { throw 'Synthetic image was not applied within startup deadline' }
        [void][GpuIdleNative]::ShowWindow($hwnd,4)
        [void][GpuIdleNative]::SetWindowPos($hwnd,[IntPtr]::Zero,80,80,1280,720,0x14)
        Start-Sleep -Seconds 5
        foreach ($stage in @('visible','dock-focus','minimized','hidden','restored')) {
            switch ($stage) {
                'dock-focus' { [void][GpuIdleNative]::PostMessage($hwnd,0x100,[IntPtr]9,[IntPtr]::Zero) }
                'minimized' { [void][GpuIdleNative]::PostMessage($hwnd,0x100,[IntPtr]27,[IntPtr]::Zero); [void][GpuIdleNative]::ShowWindow($hwnd,6) }
                'hidden' { [void][GpuIdleNative]::ShowWindow($hwnd,0) }
                'restored' { [void][GpuIdleNative]::ShowWindow($hwnd,9) }
            }
            Start-Sleep -Seconds 3
            $startQpc = [Diagnostics.Stopwatch]::GetTimestamp()
            for ($sample=0; $sample -lt $Seconds; $sample++) {
                # Instance creation/destruction can invalidate individual PDH samples; retain only valid own-PID data.
                $counter = Get-Counter -Counter '\GPU Engine(*)\Utilization Percentage' -ErrorAction SilentlyContinue -ErrorVariable counterErrors
                $own = @($counter.CounterSamples | Where-Object { $_.Status -eq 0 -and $_.InstanceName -match ('^pid_' + $process.Id + '_') })
                $maxEngine = if ($own.Count) { ($own | Measure-Object CookedValue -Maximum).Maximum } else { $null }
                $gpu = (& nvidia-smi --query-gpu=utilization.gpu,clocks.gr,power.draw,memory.used --format=csv,noheader,nounits 2>$null | Select-Object -First 1)
                $records.Add([pscustomobject]@{ Repetition=$repetition; Stage=$stage; Sample=$sample; Utc=[DateTime]::UtcNow.ToString('o'); Qpc=[Diagnostics.Stopwatch]::GetTimestamp(); ProcessId=$process.Id; EngineInstances=$own.Count; ProcessMaxEnginePercent=$maxEngine; CounterErrors=@($counterErrors).Count; DeviceSensors=$gpu })
                if ($process.HasExited) { throw 'Product exited during measurement' }
                Start-Sleep -Milliseconds 1000
            }
            $stages.Add([pscustomobject]@{Repetition=$repetition;Stage=$stage;StartQpc=$startQpc;EndQpc=[Diagnostics.Stopwatch]::GetTimestamp();Frequency=[Diagnostics.Stopwatch]::Frequency;Visible=[GpuIdleNative]::IsWindowVisible($hwnd);Minimized=[GpuIdleNative]::IsIconic($hwnd)})
            Write-Host "Completed repetition $repetition, $stage"
        }
        # The product's explicit Quit action bypasses any hide-to-tray preference.
        if (![GpuIdleNative]::PostMessage($hwnd,0x111,[IntPtr]1002,[IntPtr]::Zero)) { throw 'Could not post explicit Quit to child window' }
        if (!$process.WaitForExit(10000)) { throw 'Product did not close within 10 seconds' }
        if ($process.ExitCode -ne 0) { throw "Product exit code $($process.ExitCode)" }
        if (!(Test-Path -LiteralPath $tracePath)) { throw 'Product did not export its trace' }
    }
    $completed = $true
} finally {
    # Watchdog only owns this script's child; never terminates unrelated user processes.
    if ($process -and !$process.HasExited) {
        [void][GpuIdleNative]::PostMessage([GpuIdleNative]::FindOwnedWindow([uint32]$process.Id),0x111,[IntPtr]1002,[IntPtr]::Zero)
        if (!$process.WaitForExit(5000)) { Stop-Process -Id $process.Id -Force }
    }
    $env:IMAGE_VIEWER_PERF_OUTPUT=$savedOutput
    $env:IMAGE_VIEWER_PERF_EXIT_MS=$savedExit
    $env:IMAGE_VIEWER_PERF_READY_EXIT_MS=$savedReadyExit
    $env:IMAGE_VIEWER_PERF_SCENARIO=$savedScenario
    $records | Export-Csv -LiteralPath (Join-Path $runDirectory 'sensors.csv') -NoTypeInformation
    $stages | Export-Csv -LiteralPath (Join-Path $runDirectory 'stages.csv') -NoTypeInformation
    [pscustomobject]@{ Completed=$completed; Source=$sourceExe; SHA256=(Get-FileHash -LiteralPath (Join-Path $runDirectory 'image_viewer.exe')).Hash; CollectorSHA256=(Get-FileHash -LiteralPath $PSCommandPath).Hash; Seed='synthetic 2560x1440 gradient'; Window='1280x720 outer'; Seconds=$Seconds; Repetitions=$Repetitions; Gpu=(& nvidia-smi --query-gpu=name,driver_version --format=csv,noheader) } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $runDirectory 'manifest.json')
    Write-Host "Isolated GPU artifacts: $runDirectory"
}
