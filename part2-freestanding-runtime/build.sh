#!/usr/bin/env bash
# Part 2 — freestanding Lua injected into an already-compiled non-PIE binary.
# Requires: gcc, binutils, git, python3 + lief. Lua 5.4 source in ./lua.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"; cd "$HERE"
[ -f lua/lua.h ] || { echo "Missing ./lua — git clone --depth 1 --branch v5.4.7 https://github.com/lua/lua.git"; exit 1; }
FLAGS="-O2 -fPIC -ffreestanding -fno-stack-protector -fcf-protection=none -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0"
mkdir -p obj
[ -x lua/build/lua ] || ( cd lua && mkdir -p build
  for f in $(ls *.c | grep -vE '^(lua|luac|onelua)\.c$'); do gcc -O2 -DLUA_USE_LINUX -c "$f" -o "build/${f%.c}.o"; done
  ar rcs build/liblua.a build/*.o; gcc -O2 -DLUA_USE_LINUX lua.c build/liblua.a -lm -ldl -o build/lua )
gcc gen_ctype.c -o obj/gen_ctype && ./obj/gen_ctype > ctype_table.h
lua/build/lua gen_bytecode.lua render.lua render_bc.h >/dev/null
SET="lapi lcode lctype ldebug ldo ldump lfunc lgc llex lmem lobject lopcodes lparser lstate lstring ltable ltm lundump lvm lzio lstrlib lauxlib"
for o in $SET; do gcc $FLAGS -Ilua -c "lua/$o.c" -o "obj/$o.o"; done
gcc $FLAGS -Ilua -c shim.c -o obj/shim.o
gcc $FLAGS -c mini_libc.c -o obj/mini_libc.o
gcc $FLAGS -c mini_jmp.S  -o obj/mini_jmp.o
ld -static -T flat.ld -o lua_blob.elf obj/shim.o obj/mini_libc.o obj/mini_jmp.o $(for o in $SET; do echo obj/$o.o; done) 2>/dev/null
U=$(nm -u lua_blob.elf | wc -l); [ "$U" -eq 0 ] || { echo "ERROR: $U undefined symbols"; exit 1; }
echo "blob OK: 0 undefined symbols; instr_printf at $(nm lua_blob.elf | awk '/ instr_printf$/{print "0x"$1}')"
gcc -O0 -no-pie -fno-builtin hello.c -o hello
python3 -I inject.py hello lua_blob.elf hello_lua
chmod +x hello_lua
echo "=== hello_lua (printf via embedded freestanding Lua) ==="
./hello_lua
