#!/bin/sh
# Builds seg.cpp for wasm32-wasip2 and wasm32-wasip3 and precompiles both for wasmtime 47.
set -e
cd "$(dirname "$0")"
CXX=$HOME/dev/webcpp/.local/wasi-sdk/bin/clang++
for t in p2 p3; do
  env -u CPATH -u CPLUS_INCLUDE_PATH "$CXX" --target=wasm32-wasi$t -std=c++20 -O2 \
    -fwasm-exceptions -mllvm -wasm-use-legacy-eh=false -Wno-deprecated-declarations \
    -I/opt/homebrew/opt/boost/include -I../core seg.cpp -o seg_$t.wasm -lunwind
  wasmtime compile -W exceptions=y seg_$t.wasm -o seg_$t.cwasm
done
