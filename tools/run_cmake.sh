#!/bin/sh
set -e

build_type=$1
[ "$build_type" == "" ] && build_type=RelWithDebInfo

build_dir=build
[ ! -e "$build_dir" ] && mkdir -p "$build_dir"

pushd "$build_dir"
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=$build_type .. || true
popd
