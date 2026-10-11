#!/usr/bin/env bash
# Part 1 — link-time interception with ld --wrap. Needs only gcc + binutils.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"; cd "$HERE"

echo "== what main actually calls at -O2 (printf -> puts / __printf_chk) =="
gcc -O2 hello.c -o hello_O2
objdump -d hello_O2 | awk '/<main>:/{f=1} f&&/call/{print} /ret/{if(f)exit}'

echo "== with -O0 -fno-builtin (printf stays printf) =="
gcc -O0 -fno-builtin hello.c -o hello_plain
objdump -d hello_plain | awk '/<main>:/{f=1} f&&/call/{print} /ret/{if(f)exit}'

echo "== wrapped: every printf goes through __wrap_printf =="
gcc -O0 -fno-builtin -Wl,--wrap=printf hello.c wrap.c -o hello_wrapped
./hello_wrapped
objdump -d hello_wrapped | awk '/<main>:/{f=1} f&&/call/{print} /ret/{if(f)exit}'
