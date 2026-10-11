#!/usr/bin/env python3
"""Multi-symbol injector: redirects several imported symbols, each to its own
function in the blob. Works on a PIE target (the blob self-relocates) and finds
everything through the PROGRAM HEADERS, so it also works on a binary with no
section table. Adding a symbol = one map entry + an instr_<x> in the shim + a
Lua function in the render.

Usage: python3 -I inject_multi.py <target> <blob.so> <output>
"""
import sys, struct, lief

# imported symbol in the target  ->  entry function in the blob
TARGETS = {"printf": "instr_printf", "puts": "instr_puts"}

def x_segments(b):
    return [s for s in b.segments
            if s.type == lief.ELF.Segment.TYPE.LOAD and lief.ELF.Segment.FLAGS.X in s.flags]

def got_of(b, name):
    for r in b.pltgot_relocations:          # comes from PT_DYNAMIC, not a section
        s = getattr(r, "symbol", None)
        if s and s.name == name: return r.address
    for r in b.relocations:
        s = getattr(r, "symbol", None)
        if s and s.name == name: return r.address
    return None

def plt_of(b, got):
    ENDBR = bytes.fromhex("f30f1efa")
    for seg in x_segments(b):
        base, data = seg.virtual_address, bytes(seg.content)
        for off in range(len(data) - 6):
            if data[off] == 0xFF and data[off+1] == 0x25:     # jmp qword [rip+rel32]
                rel = struct.unpack_from("<i", data, off+2)[0]
                if base + off + 6 + rel == got:
                    return base + (off-4 if off>=4 and data[off-4:off]==ENDBR else off)
    return None

def sites_of(b, target):
    out = []
    for seg in x_segments(b):
        base, data = seg.virtual_address, bytes(seg.content)
        for off in range(len(data) - 5):
            if data[off] == 0xE8 and base + off + 5 + struct.unpack_from("<i", data, off+1)[0] == target:
                out.append(base + off)
    return out

tgt, blob_path, out = sys.argv[1], sys.argv[2], sys.argv[3]

blob = lief.parse(blob_path)
entry_off = {name: blob.get_symbol(fn).value for name, fn in TARGETS.items()}
loads = [s for s in blob.segments if s.type == lief.ELF.Segment.TYPE.LOAD]
end = max(s.virtual_address + max(s.virtual_size, len(bytes(s.content))) for s in loads)
flat = bytearray(end)
for s in loads:
    c = bytes(s.content); flat[s.virtual_address:s.virtual_address+len(c)] = c

# STEP 1: inject the segment and write (LIEF may rebase the whole ET_DYN)
b = lief.parse(tgt)
seg = lief.ELF.Segment()
seg.type = lief.ELF.Segment.TYPE.LOAD
seg.add(lief.ELF.Segment.FLAGS.R); seg.add(lief.ELF.Segment.FLAGS.W); seg.add(lief.ELF.Segment.FLAGS.X)
seg.content = list(flat)
b.add(seg); b.write(out)

# STEP 2: reopen the now-stable file and patch at the FINAL coordinates
b = lief.parse(out)
base = None
FL = lief.ELF.Segment.FLAGS
for s in b.segments:                       # the injected segment is the only RWX one
    if s.type == lief.ELF.Segment.TYPE.LOAD and FL.W in s.flags and FL.X in s.flags \
       and len(bytes(s.content)) >= len(flat):
        base = s.virtual_address; break
if base is None: raise SystemExit("injected (RWX) segment not found")

total = 0
for name, fn in TARGETS.items():
    got = got_of(b, name)
    if got is None:
        print(f"  {name}: not imported by target (skipped)"); continue
    plt = plt_of(b, got)
    if plt is None:
        print(f"  {name}: GOT {got:#x} but no PLT stub (skipped)"); continue
    entry = base + entry_off[name]
    sites = sites_of(b, plt)
    print(f"  {name:8} GOT={got:#x} plt={plt:#x} -> {fn}@{entry:#x}  sites={[hex(x) for x in sites]}")
    for x in sites:
        b.patch_address(x + 1, list(struct.pack("<i", entry - (x + 5))))
    total += len(sites)

if total == 0: raise SystemExit("no call site redirected")
b.write(out); print(f"wrote: {out} ({total} call sites)")
