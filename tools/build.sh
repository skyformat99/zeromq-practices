#!/bin/sh
set -e

build_dir=build

for target in $@; do
    cmake --build "$build_dir" --use-stderr --target "$target" || true
done
