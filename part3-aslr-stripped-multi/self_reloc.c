/* self_reloc.c - self-relocation for the injected PIE blob (no dependency on the
 * target's ld.so). Finds the load bias and applies the blob's own
 * R_X86_64_RELATIVE entries, exactly like the ld.so bootstrap. Runs once,
 * before Lua is used. */

typedef unsigned long u64;
typedef long          i64;

typedef struct { i64 d_tag; u64 d_un; } Dyn;
typedef struct { u64 r_offset; u64 r_info; i64 r_addend; } Rela;

#define DT_NULL     0
#define DT_RELA     7
#define DT_RELASZ   8
#define DT_RELAENT  9
#define R_X86_64_RELATIVE 8
#define R_TYPE(info) ((info) & 0xffffffffUL)

/* hidden => the compiler uses RIP-relative lea (not the GOT), which works
 * BEFORE relocation has run */
extern Dyn _DYNAMIC[] __attribute__((visibility("hidden")));
extern u64 _GLOBAL_OFFSET_TABLE_[] __attribute__((visibility("hidden")));

void self_relocate(void){
    /* base = (&_DYNAMIC at runtime, via RIP) - (&_DYNAMIC at link time, in GOT[0]) */
    u64 base = (u64)&_DYNAMIC[0] - _GLOBAL_OFFSET_TABLE_[0];

    Rela *rela = 0; u64 sz = 0, ent = sizeof(Rela);
    for (Dyn *d = _DYNAMIC; d->d_tag != DT_NULL; d++) {
        if      (d->d_tag == DT_RELA)    rela = (Rela*)(base + d->d_un);
        else if (d->d_tag == DT_RELASZ)  sz   = d->d_un;
        else if (d->d_tag == DT_RELAENT) ent  = d->d_un;
    }
    for (u64 i = 0; i < sz; i += ent) {
        Rela *r = (Rela*)((char*)rela + i);
        if (R_TYPE(r->r_info) == R_X86_64_RELATIVE)
            *(u64*)(base + r->r_offset) = base + (u64)r->r_addend;
    }
}
