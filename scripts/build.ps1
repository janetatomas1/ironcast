# Builds ironcast with a compiler whose dependencies submodules/conanfiles/install.py has deployed.
# Usage: scripts\build.ps1 <msvc|clang-cl> [--debug] [--release]
# Builds debug when neither is given, both when both are. Output goes to build\<compiler>-<type>.
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot

function Show-Usage {
    Write-Host 'usage: scripts\build.ps1 <msvc|clang-cl> [--debug] [--release]'
    exit 1
}

if ($args.Count -lt 1) {
    Show-Usage
}
$compiler = $args[0]

$debug = $false
$release = $false
foreach ($arg in ($args | Select-Object -Skip 1)) {
    switch ($arg) {
        '--debug' { $debug = $true }
        '--release' { $release = $true }
        default { Show-Usage }
    }
}
if (-not $debug -and -not $release) {
    $debug = $true
}

$types = @()
if ($debug) { $types += 'Debug' }
if ($release) { $types += 'Release' }

foreach ($type in $types) {
    $deploy = Join-Path $root "submodules\conanfiles\third-party\$compiler\$($type.ToLower())"
    $toolchain = Join-Path $deploy 'conan_toolchain.cmake'
    if (-not (Test-Path $toolchain)) {
        Write-Host "missing $toolchain"
        Write-Host "run in submodules\conanfiles: python install.py --compilers $compiler --build-types $type"
        exit 1
    }
    $build = Join-Path $root "build\$compiler-$($type.ToLower())"
    $configure = "cmake -S `"$root`" -B `"$build`" -G Ninja -DCMAKE_BUILD_TYPE=$type -DCMAKE_TOOLCHAIN_FILE=`"$toolchain`""
    $compile = "cmake --build `"$build`""

    # Conan writes conanvcvars.bat for MSVC toolchains: Ninja needs the Visual Studio environment it sets up.
    $vcvars = Join-Path $deploy 'conanvcvars.bat'
    if (Test-Path $vcvars) {
        cmd /c "call `"$vcvars`" && $configure && $compile"
    } else {
        cmd /c "$configure && $compile"
    }
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}
