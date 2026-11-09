# Installs ironcast's dependencies with Conan for a compiler, through submodules\conanfiles\install.py.
# Usage: scripts\install.ps1 <msvc|clang-cl> [--debug] [--release]
# Installs debug when neither is given, both when both are. scripts\build.ps1 then builds with them.
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot

function Show-Usage {
    Write-Host 'usage: scripts\install.ps1 <msvc|clang-cl> [--debug] [--release]'
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

# Only when it isn't checked out: update would also move an existing checkout back to the recorded commit.
if (-not (Test-Path (Join-Path $root 'submodules\conanfiles\install.py'))) {
    git -C $root submodule update --init submodules/conanfiles
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

# install.py finds the recipes and writes third-party\ relative to its own directory.
Push-Location (Join-Path $root 'submodules\conanfiles')
try {
    python install.py --compilers $compiler --build-types @types
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
} finally {
    Pop-Location
}
