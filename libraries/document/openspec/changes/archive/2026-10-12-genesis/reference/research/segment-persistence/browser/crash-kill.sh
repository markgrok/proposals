#!/bin/sh
# SIGKILL of the whole browser (every process of its tree, and WebKit's XPC services) with a
# persistent profile, 200 kills, every strategy running at once; the three engines in parallel.
cd "$(dirname "$0")"
node run.mjs sigkill chromium port=8721 > logs/sigkill-chromium.log 2>&1 &
node run.mjs sigkill webkit port=8722 > logs/sigkill-webkit.log 2>&1 &
node run.mjs sigkill firefox port=8723 > logs/sigkill-firefox.log 2>&1 &
wait
