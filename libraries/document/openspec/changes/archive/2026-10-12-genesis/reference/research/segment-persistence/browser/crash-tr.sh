#!/bin/sh
# worker.terminate() and page reload, 200 kills per strategy, the three engines in parallel.
cd "$(dirname "$0")"
node run.mjs terminate chromium port=8701 > logs/terminate-chromium.log 2>&1 &
node run.mjs terminate webkit port=8702 > logs/terminate-webkit.log 2>&1 &
node run.mjs terminate firefox port=8703 > logs/terminate-firefox.log 2>&1 &
wait
node run.mjs reload chromium port=8711 > logs/reload-chromium.log 2>&1 &
node run.mjs reload webkit port=8712 > logs/reload-webkit.log 2>&1 &
node run.mjs reload firefox port=8713 > logs/reload-firefox.log 2>&1 &
wait
