#!/bin/sh
set -eu
cd "$(dirname "$0")"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build --parallel 3
ctest --test-dir build --output-on-failure
# The linker's signature does not seal the bundle; Logic and other AU hosts can refuse it.
OUT=build/SwellAcousticGuitar_artefacts/Release
for b in "$OUT/AU/Swell Guitar.component" "$OUT/VST3/Swell Guitar.vst3" "$OUT/Standalone/Swell Guitar.app"; do codesign --force --deep -s - "$b"; done
