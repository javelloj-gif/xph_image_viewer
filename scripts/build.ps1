param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [string]$CMakePath,
    [string]$BuildDirectory,
    [switch]$Test
)
. (Join-Path $PSScriptRoot 'common.ps1')
$cmake = Get-CMake $CMakePath
if (!$BuildDirectory) { $BuildDirectory = Join-Path $ProjectRoot 'build/windows-x64' }
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
& $cmake -S $ProjectRoot -B $BuildDirectory -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON
Assert-NativeSuccess 'CMake configure'
& $cmake --build $BuildDirectory --config $Configuration
Assert-NativeSuccess 'CMake build'
if ($Test) {
    $ctest = Resolve-Tool 'ctest.exe' '' @((Join-Path (Split-Path -Parent $cmake) 'ctest.exe'))
    & $ctest --test-dir $BuildDirectory -C $Configuration --output-on-failure
    Assert-NativeSuccess 'CTest'
}
