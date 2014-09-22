#!/bin/sh
set -e

target="$1"
build_dir="$2"
[ "$target" == "" ] && target=all
[ "$build_dir" == "" ] && build_dir=build

shift && shift || true
extra_opt="$@"
[ ! "$extra_opt" == "" ] && extra_opt="-- $extra_opt"

cmake --build "$build_dir" --use-stderr --target "$target" $extra_opt
