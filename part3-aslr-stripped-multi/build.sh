#!/usr/bin/env bash
# Part 3 - self-relocating freestanding Lua injected into a PIE binary with NO
# section table, intercepting both printf and puts.
# Requires: gcc, binutils, git, python3 + lief. Lua 5.4 source in ./lua.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"; cd "$HERE"
[ -f lua/lua.h ] || { echo "Missing ./lua - git clone --depth 1 --branch v5.4.7 https://github.com/lua/lua.git"; exit 1; }
FLAGS="-O2 -fPIC -ffreestanding -fno-stack-protector -fcf-protection=none -fvisibility=hidden -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0"
mkdir -p obj
[ -x lua/build/lua ] || ( cd lua && mkdir -p build
  for f in $(ls *.c | grep -vE '^(lua|luac|onelua)\.c$'); do gcc -O2 -DLUA_USE_LINUX -c "$f" -o "build/${f%.c}.o"; done
  ar rcs build/liblua.a build/*.o; gcc -O2 -DLUA_USE_LINUX lua.c build/liblua.a -lm -ldl -o build/lua )
gcc gen_ctype.c -o obj/gen_ctype && ./obj/gen_ctype > ctype_table.h
lua/build/lua gen_bytecode.lua render_multi.lua render_bc.h >/dev/null
SET="lapi lcode lctype ldebug ldo ldump lfunc lgc llex lmem lobject lopcodes lparser lstate lstring ltable ltm lundump lvm lzio lstrlib lauxlib"
for o in $SET; do gcc $FLAGS -Ilua -c "lua/$o.c" -o "obj/$o.o"; done
gcc $FLAGS -Ilua -c shim_multi.c -o obj/shim_multi.o
gcc $FLAGS -c mini_libc.c  -o obj/mini_libc.o
gcc $FLAGS -c mini_jmp.S    -o obj/mini_jmp.o
gcc $FLAGS -c self_reloc.c  -o obj/self_reloc.o
# shared object: -Bsymbolic + hidden visibility => only R_X86_64_RELATIVE relocs
ld -shared -Bsymbolic --no-dynamic-linker -o lua_multi.so \
   obj/shim_multi.o obj/mini_libc.o obj/mini_jmp.o obj/self_reloc.o $(for o in $SET; do echo obj/$o.o; done) 2>/dev/null
NONREL=$(readelf -r lua_multi.so 2>/dev/null | grep -oE 'R_X86_64_[A-Z_]+' | grep -cv RELATIVE || true)
U=$(nm -u lua_multi.so | wc -l)
[ "$NONREL" -eq 0 ] && [ "$U" -eq 0 ] || { echo "ERROR: non-RELATIVE relocs=$NONREL undefined=$U"; exit 1; }
echo "blob OK: entries = $(nm lua_multi.so | grep -oE 'instr_(printf|puts)' | tr '\n' ' ')"
# PIE target, then strip the section table (e_shoff/e_shnum/e_shstrndx = 0)
gcc -O0 -fPIE -pie -fno-builtin hello_multi.c -o hello_pie
cp hello_pie hello_nosh
python3 -I - <<'PY'
import struct
f=bytearray(open("hello_nosh","rb").read())
struct.pack_into("<Q",f,0x28,0); struct.pack_into("<H",f,0x3c,0); struct.pack_into("<H",f,0x3e,0)
open("hello_nosh","wb").write(f)
PY
echo "target with no sections: $(readelf -h hello_nosh 2>/dev/null | awk '/Number of section/{print $NF}') section headers"
python3 -I inject_multi.py hello_nosh lua_multi.so hello_rw
chmod +x hello_rw
echo "=== hello_rw (printf and puts via Lua, no sections, PIE+ASLR) ==="
./hello_rw
