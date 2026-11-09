#!/usr/bin/env bash
# Installs ironcast's dependencies with Conan for a compiler, through submodules/conanfiles/install.py.
# Usage: scripts/install.sh <gcc|clang|emscripten> [--debug] [--release]
# Installs debug when neither is given, both when both are. scripts/build.sh then builds with them.
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

# Only when it isn't checked out: update would also move an existing checkout back to the recorded commit.
if [[ ! -f $root/submodules/conanfiles/install.py ]]; then
    git -C "$root" submodule update --init submodules/conanfiles
fi
# install.py finds the recipes and writes third-party/ relative to its own directory.
cd "$root/submodules/conanfiles"
python3 install.py --compilers "$compiler" --build-types "${types[@]}"
