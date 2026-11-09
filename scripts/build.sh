#!/usr/bin/env bash
# Builds ironcast with a compiler whose dependencies submodules/conanfiles/install.py has deployed.
# Usage: scripts/build.sh <gcc|clang|emscripten> [--debug] [--release]
# Builds debug when neither is given, both when both are. Output goes to build/<compiler>-<type>.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

usage() {
    echo "usage: $0 <gcc|clang|emscripten> [--debug] [--release]" >&2
    exit 1
}

[[ $# -ge 1 ]] || usage
compiler=$1
shift

debug=0
release=0
for arg in "$@"; do
    case $arg in
        --debug) debug=1 ;;
        --release) release=1 ;;
        *) usage ;;
    esac
done
if [[ $debug == 0 && $release == 0 ]]; then
    debug=1
fi

types=()
[[ $debug == 1 ]] && types+=(Debug)
[[ $release == 1 ]] && types+=(Release)

for type in "${types[@]}"; do
    toolchain="$root/submodules/conanfiles/third-party/$compiler/${type,,}/conan_toolchain.cmake"
    if [[ ! -f $toolchain ]]; then
        echo "missing $toolchain" >&2
        echo "run in submodules/conanfiles: python install.py --compilers $compiler --build-types $type" >&2
        exit 1
    fi
    build="$root/build/$compiler-${type,,}"
    cmake -S "$root" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE="$type" -DCMAKE_TOOLCHAIN_FILE="$toolchain"
    cmake --build "$build"
done
