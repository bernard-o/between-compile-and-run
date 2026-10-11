#!/usr/bin/env python3
"""Inject a freestanding blob (linked at a fixed VA) into an already-compiled,
non-PIE ELF, and redirect printf to the blob's instr_printf. No relink.
Usage: python3 -I inject.py <target> <blob.elf> <output>"""
import sys, struct, lief

def printf_plt(b):
    got = next((r.address for r in b.relocations
                if getattr(r,"symbol",None) and r.symbol.name=="printf"), None)
    if got is None: raise SystemExit("printf not in relocations")
    ENDBR = bytes.fromhex("f30f1efa")
    for name in (".plt.sec",".plt"):
        s=b.get_section(name)
        if not s: continue
        base,data=s.virtual_address,bytes(s.content)
        for off in range(len(data)-6):
            if data[off]==0xFF and data[off+1]==0x25:
                rel=struct.unpack_from("<i",data,off+2)[0]
                if base+off+6+rel==got:
                    return base+(off-4 if off>=4 and data[off-4:off]==ENDBR else off)
    raise SystemExit("printf@plt not found")

def call_sites(b,target):
    t=b.get_section(".text"); base,data=t.virtual_address,bytes(t.content)
    return [base+o for o in range(len(data)-5)
            if data[o]==0xE8 and base+o+5+struct.unpack_from("<i",data,o+1)[0]==target]

tgt,blob_path,out=sys.argv[1],sys.argv[2],sys.argv[3]
blob=lief.parse(blob_path)
entry=blob.get_symbol("instr_printf").value
loads=[s for s in blob.segments if s.type==lief.ELF.Segment.TYPE.LOAD]
plt=printf_plt(lief.parse(tgt))
print(f"printf@plt={plt:#x}  instr_printf={entry:#x}")

b=lief.parse(tgt)
for s in loads:
    seg=lief.ELF.Segment(); seg.type=lief.ELF.Segment.TYPE.LOAD
    seg.add(lief.ELF.Segment.FLAGS.R); seg.add(lief.ELF.Segment.FLAGS.W); seg.add(lief.ELF.Segment.FLAGS.X)
    seg.virtual_address=s.virtual_address
    seg.content=list(bytes(s.content))
    ns=b.add(seg)
    if ns.virtual_address!=s.virtual_address: raise SystemExit("VA not honored")
sites=call_sites(b,plt); print(f"call sites: {[hex(x) for x in sites]}")
for x in sites: b.patch_address(x+1, list(struct.pack("<i", entry-(x+5))))
b.write(out); print("wrote", out)
