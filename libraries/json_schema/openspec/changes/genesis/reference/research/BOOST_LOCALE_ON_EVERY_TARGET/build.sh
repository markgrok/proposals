#!/bin/bash
# Usage: build.sh <target> <backends: posix-std | std> ; builds Boost.Locale 1.92 (no ICU, iconv) and Charconv, links the probe.
unset CPATH CPLUS_INCLUDE_PATH C_INCLUDE_PATH
t=$1; v=$2
S=~/.claude/jobs/fcf3a902/tmp/clone/.build/third_party/boost
I=/opt/homebrew/opt/boost/include
SDK=$HOME/dev/webcpp/.local/wasi-sdk/bin
EH="-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false"
LIBS=""
case $t in
  clang) CXX="clang++"; AR=ar; LIBS="-liconv";;
  gcc15) CXX="g++-15 -D_Static_assert=static_assert"; AR=ar; LIBS="-liconv";;
  wasip2) CXX="$SDK/clang++ --target=wasm32-wasip2 $EH"; AR=$SDK/llvm-ar; LIBS="-lunwind";;
  wasip3) CXX="$SDK/clang++ --target=wasm32-wasip3 $EH"; AR=$SDK/llvm-ar; LIBS="-lunwind";;
  emscripten) CXX="$HOME/dev/webcpp/.local/emscripten/em++ -fwasm-exceptions"; AR=$HOME/dev/webcpp/.local/emsdk/upstream/emscripten/emar; LIBS="-sNODERAWFS=1";;
esac
SRC="encoding/codepage shared/date_time shared/format shared/formatting shared/generator shared/iconv_codecvt shared/ids
     shared/localization_backend shared/message shared/mo_lambda util/codecvt_converter util/default_locale util/encoding
     util/info util/locale_data util/gregorian std/codecvt std/collate std/converter std/numeric std/std_backend"
DEFS="-DBOOST_LOCALE_SOURCE -DBOOST_LOCALE_NO_WINAPI_BACKEND=1 -DBOOST_LOCALE_WITH_ICONV=1"
case $v in
  posix-std) SRC="$SRC posix/collate posix/converter posix/numeric posix/codecvt posix/posix_backend";;
  std) DEFS="$DEFS -DBOOST_LOCALE_NO_POSIX_BACKEND=1";;
esac
out=obj-$t-$v; rm -rf $out; mkdir -p $out; ok=0; bad=0
for s in $SRC; do
  o=$out/$(echo $s | tr / _).o
  if $CXX -std=c++20 -O2 $DEFS -I$I -I$S/libs/locale/src -c $S/libs/locale/src/$s.cpp -o $o 2> $o.err; then ok=$((ok+1)); else bad=$((bad+1)); echo "  FAIL $s: $(grep -m1 'error' $o.err)"; fi
done
for s in from_chars to_chars; do
  o=$out/charconv_$s.o
  if $CXX -std=c++20 -O2 -DBOOST_CHARCONV_SOURCE=1 -I$I -I$S/libs/charconv/src -c $S/libs/charconv/src/$s.cpp -o $o 2> $o.err; then ok=$((ok+1)); else bad=$((bad+1)); echo "  FAIL charconv/$s: $(grep -m1 'error' $o.err)"; fi
done
rm -f lib-$t-$v.a; $AR rcs lib-$t-$v.a $out/*.o
echo "$t $v: compiled $ok, failed $bad"
case $t in wasip2|wasip3) ext=.wasm;; emscripten) ext=.js;; *) ext=;; esac
if $CXX -std=c++20 -O2 -I$I locale_probe.cpp lib-$t-$v.a $LIBS -o probe-$t-$v$ext 2> link-$t-$v.err; then echo "  linked"; else echo "  LINK FAIL: $(grep -m3 -i 'error\|undefined' link-$t-$v.err | head -3)"; fi
