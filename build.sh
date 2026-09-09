#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

run_examples=1
if [ "${1:-}" = "-norun" ]; then
    run_examples=0
    shift
fi

slang_version=2026.17
slang_dir="$root/tools/slang-$slang_version"
slang_bin="$slang_dir/bin"
slang_tarball="$root/tools/slang-$slang_version-linux-x86_64.tar.gz"

if [ ! -x "$slang_bin/slangc" ]; then
    if [ ! -f "$slang_tarball" ]; then
        curl -sfL -o "$slang_tarball" \
            "https://github.com/shader-slang/slang/releases/download/v$slang_version/slang-$slang_version-linux-x86_64.tar.gz"
    fi
    mkdir -p "$slang_dir"
    tar -xzf "$slang_tarball" -C "$slang_dir"
    rm "$slang_tarball"
fi

export PATH="$slang_bin:$PATH"

options=(-D CMAKE_BUILD_TYPE=Release -D NOGRAPHICSAPI_SLANGC="$slang_bin/slangc")
case "$(uname -s)" in
    MINGW*|CYGWIN*|Windows*) ;;
    *) options+=(-D NOGRAPHICSAPI_BUILD_EXAMPLES=OFF -D NOGRAPHICSAPI_BUILD_EXAMPLES_GLFW=ON) ;;
esac

cmake -S "$root" -B "$root/build" "${options[@]}" "$@"
cmake --build "$root/build" -j "$(nproc)"

if [ "$run_examples" -eq 1 ]; then
    for name in example_triangle_glfw example_cube_glfw example_deferred_renderer_glfw; do
        for example in "$root"/build/examples_glfw/*/"$name"; do
            [ -x "$example" ] && "$example"
        done
    done
fi
