# Part 3 — Surviving ASLR, no sections, two functions

Companion code for the article *Surviving ASLR on a Stripped Binary*.

```bash
pip install --break-system-packages lief
git clone --depth 1 --branch v5.4.7 https://github.com/lua/lua.git
./build.sh
```

`build.sh` links the blob as a position-independent shared object (so its only
relocations are `R_X86_64_RELATIVE`), builds a **PIE** target, strips its section
table, injects, and redirects both `printf` and `puts`. Expected output (stable
across runs under full ASLR):

```
[lua:puts] opening (via puts)
[lua:printf] processed 3 files in 12.5 s
[lua:puts] closing (via puts)
```

- `self_reloc.c` — on first use, finds the load bias and applies the blob's own
  `R_X86_64_RELATIVE` entries, so absolute pointers are correct under ASLR. No
  dependency on the target's `ld.so`.
- `inject_multi.py` — finds `printf`/`puts`, their PLT stubs and call sites through
  the **program headers** (`PT_DYNAMIC`), so it works with no section table.
- `shim_multi.c` / `render_multi.lua` — one handler per intercepted symbol.

**Adding a symbol** is three edits: a Lua function in `render_multi.lua`, an
`instr_<x>` in `shim_multi.c`, and an entry in `TARGETS` in `inject_multi.py`.

**Why this isn't stealthy:** adding an RWX segment and rewriting call sites is
exactly the shape endpoint protection looks for. This is instrumentation for your
own binaries, not an evasion technique. The real tools (Frida, DynamoRIO) take
very different, less detectable approaches.
