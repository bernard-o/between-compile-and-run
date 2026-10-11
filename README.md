# Between Compile and Run

A hands-on series on **binary instrumentation**: understanding the ELF loader and
the linker by building a tool that redirects C-library calls (`printf`, `puts`)
into a small **Lua runtime embedded inside an already-compiled binary** — with no
relink, surviving ASLR, and working on a binary with no symbols and no section
table.

Each part is self-contained and maps to one article in the series.

> **What this is, and isn't.** This is binary instrumentation on *your own*
> binaries — the same family of techniques as [LIEF](https://lief.re),
> [Frida](https://frida.re) and DynamoRIO, which power profilers, debuggers,
> coverage tools and malware *analysis*. The goal here is to understand the ELF
> format and the loader well enough to build the tool. It is not an evasion
> technique: as Part 3 discusses, static call-site patching is exactly what
> endpoint protection flags first. Nothing here targets software you don't own.

## The series

| Part | Folder | What it covers |
|---|---|---|
| 1 | [`part1-link-time-wrap/`](part1-link-time-wrap/) | What `main` really calls; PLT/GOT; interception at link time with `ld --wrap`. |
| 2 | [`part2-freestanding-runtime/`](part2-freestanding-runtime/) | A libc-free Lua runtime injected into an already-compiled (non-PIE) binary. |
| 3 | [`part3-aslr-stripped-multi/`](part3-aslr-stripped-multi/) | Self-relocation to survive ASLR; finding call sites through program headers on a stripped PIE; intercepting `printf` and `puts`. |

Article links: *(to be filled in on publication)*
- Part 1 — `[<PART1_URL>](https://medium.com/@ceballosyalba/the-function-you-called-isnt-the-function-that-ran-c3e61a874640)`
- Part 2 — `[<PART2_URL>](https://medium.com/@ceballosyalba/a-runtime-with-no-libc-injected-into-a-finished-binary-8c6b055e7c08)`
- Part 3 — `[<PART3_URL>](https://medium.com/@ceballosyalba/surviving-aslr-on-a-stripped-binary-59e7a040aab2)`

## Environment

Everything here was built and tested on this exact setup. The concepts hold
elsewhere, but specific addresses, and whether `__printf_chk` appears (it depends
on the distro's `_FORTIFY_SOURCE` default), will vary.

| Component | Version |
|---|---|
| OS | Ubuntu 24.04.5 LTS, x86-64 |
| Compiler / linker | gcc 13.3.0, binutils 2.42 |
| Lua | 5.4.7 (source, tag `v5.4.7` from `github.com/lua/lua`) |
| LIEF | 1.0.0 |
| Python | 3.13 |

## Quick start

```bash
# Part 1 needs only gcc + binutils:
cd part1-link-time-wrap && ./build.sh

# Parts 2 and 3 need the Lua source and LIEF:
pip install --break-system-packages lief
cd part2-freestanding-runtime
git clone --depth 1 --branch v5.4.7 https://github.com/lua/lua.git
./build.sh
```

The Lua source is **not** vendored here; each part's `build.sh` expects a `lua/`
directory (the `git clone` above). `lua.org`'s download server is blocked on some
networks, so the GitHub mirror is the reliable source.

## Target assumptions

The injector targets **x86-64** ELF binaries where `printf`/`puts` are
**dynamically linked** (there is a PLT/GOT slot to anchor on). Part 2 uses a
non-PIE target; Part 3 handles PIE and a stripped section table. Statically linked
or inlined libc calls have no stub to redirect.

## License

MIT — see [LICENSE](LICENSE).
