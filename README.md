# MultiFOCAL

Local variable scoping for FOCAL — the native programming language of the
HP-41C/CV/CX calculators — via subroutine-local storage frames.

## What this is

Native FOCAL has one flat, global register file: every subroutine that
wants scratch storage has to pick numbered registers by hand, and nothing
stops two subroutines (or two levels of the same recursive subroutine)
from silently colliding over the same register. MultiFOCAL adds real
subroutine-local storage — frames that don't collide across subroutines
and disappear automatically on return — without changing how existing,
unextended FOCAL programs behave.

It's the first of two independently-timed sub-projects. The second,
working name **ConFOCAL** (backup name **VariFOCAL**), will add typed/
structured data on top of MultiFOCAL later; that's out of scope here, and
neither project is a prerequisite for the other.

## Design constraints

- **Stay on original HP-41 hardware as long as possible.** Any design
  choice that would require leaving it (custom silicon, an emulator-only
  target) gets flagged and discussed explicitly rather than defaulted into.
- **Additive only.** Native FOCAL programs must behave identically whether
  or not MultiFOCAL is present or in use — new callable functions, not a
  change to existing syntax or opcodes.
- **A standalone module.** MultiFOCAL ships as its own MCODE ROM image with
  its own Function Address Table (FAT) — the same distribution model as
  PPC ROM or the Advantage Pac, loaded into one of the four module ports.
- **Coexist with the popular existing module library.** Function-name and
  register-range collisions with widely-used modules (PPC ROM, CCD,
  Extended Functions, HEPAX, and friends) are treated as real bugs, not
  acceptable losses.
- Where a design question has no clear technical answer, this project
  prefers the HP-41 community's own de facto convention over inventing
  something new.

## Status

**Phase 0 (toolchain) is done.** The full assemble → link → load → execute
loop is proven end to end:

- [Calypsi](https://github.com/hth313/Calypsi-tool-chains) (NutStudio's
  actively-maintained successor, itself SDK41's modern replacement)
  assembles and links real MCODE into a real MOD1 module file.
- A throwaway proof-of-concept module (`src/mftest.s`) built this way was
  loaded into a headless build of a real Nut CPU emulator core, booted
  against the genuine HP-41 OS ROM, and reached by an actual
  `XEQ ALPHA MFTEST ALPHA` keystroke sequence — confirmed by the routine's
  own message appearing on the simulated display.

See `CLAUDE.md` for the full session-by-session status, open design
decisions, and what's next (a popular-modules compatibility survey,
community naming-convention research, and Phase 1's storage-design
decisions).

## Building and testing

Requires the [`gh`](https://cli.github.com/) CLI (to fetch the Calypsi
toolchain release) and a sibling checkout of
[`soynut`](https://github.com/jacob-rn-wallace/soynut) at `../soynut`,
including its own "bring your own ROM" HP-41 OS images — see
`toolchain/README.md` and `../soynut/roms/README.md`.

```
make mod    # assemble/link src/*.s -> a MOD1 file -> a ROM C array
make test   # ...then run the headless emulator loop test against it
```

## What's not included

- **No HP-41 ROM firmware.** The genuine OS ROM and any real module images
  this project tests against are HP's copyrighted calculator firmware, not
  open source, and aren't distributed here — see `../soynut/roms/README.md`
  for how to supply your own.
- **The Calypsi toolchain binaries** aren't vendored (see
  `toolchain/README.md` for how to fetch them) — they're a large,
  independently-licensed third-party download, not this project's code.

## License

GPL-2.0-or-later (see `LICENSE`). This isn't an arbitrary default: the
`test/` harness statically links object code compiled from
[`emu41gcc`](https://github.com/mmoller2k/emu41gcc) (via a read-only,
sibling checkout of `soynut`), which is itself GPL-2.0 — so the combined
work has to carry compatible terms. `soynut` hit the identical situation
for the same reason and reached the same license.
