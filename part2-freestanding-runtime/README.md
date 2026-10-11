# Part 2 — A freestanding Lua runtime, injected into a compiled binary

Companion code for the article *A Runtime With No libc, Injected Into a Finished
Binary*.

```bash
pip install --break-system-packages lief
git clone --depth 1 --branch v5.4.7 https://github.com/lua/lua.git
./build.sh
```

`build.sh` compiles Lua's core freestanding (no libc) against `mini_libc.c`, links
the blob at a fixed virtual address, injects it into an already-compiled **non-PIE**
`hello`, and redirects `printf` to the embedded Lua. Expected output:

```
[lua] starting
[lua] processed 3 files in 12.5 s
```

- `mini_libc.c` — the freestanding libc (syscall I/O, arena allocator over `mmap`,
  `mem/str`, and a `snprintf` whose output matches glibc byte for byte).
- `shim.c` — `instr_printf` decodes the varargs and calls a Lua function.
- `render.lua` — the replacement for `printf`, in Lua. **Edit this** to change the
  target's behavior, then rebuild — no need to recompile the target.
- `inject.py` — adds the blob as a new (RWX) segment and rewrites the call sites.

**Note:** the injected segment must be writable — the blob has mutable globals
(the Lua state, etc.). Mapping it read-execute only will segfault on first use.

**Limits:** the target is non-PIE (fixed load address), and we use its symbols and
sections to find `printf`. Part 3 removes both assumptions.
