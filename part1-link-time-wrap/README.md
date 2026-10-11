# Part 1 — Interception at link time (`ld --wrap`)

Companion code for the article *The Function You Called Isn't the Function That
Ran*. Needs only `gcc` and `binutils`.

```bash
./build.sh
```

It shows three things:

1. **What `main` really calls at `-O2`** — `printf("starting\n")` becomes `puts`,
   and `printf("...%d...")` becomes `__printf_chk` (the latter only when the
   toolchain enables `_FORTIFY_SOURCE`, as Ubuntu does by default).
2. **With `-O0 -fno-builtin`**, both stay `printf@plt`.
3. **With `-Wl,--wrap=printf`**, every `printf` is redirected to `__wrap_printf`,
   which counts and tags each call and delegates to `__real_printf`.

Expected output of the wrapped program:

```
[printf #1] starting
[printf #2] processed 3 files
```

**Limit:** `--wrap` happens at link time, so it needs the source and a relink.
Parts 2 and 3 work on a binary that is already compiled.
