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

**Phases 0-4 are complete** (Phases 2 and 3 tagged `phase-2`/`phase-3`).

- **Phase 0**: the full assemble → link → load → execute loop, proven
  end to end — [Calypsi](https://github.com/hth313/Calypsi-tool-chains)
  (NutStudio's actively-maintained successor, itself SDK41's modern
  replacement) assembles and links real MCODE into a real MOD1 module
  file, loaded into a headless build of a real Nut CPU emulator core,
  booted against the genuine HP-41 OS ROM, and reached by an actual
  `XEQ ALPHA <name> ALPHA` keystroke sequence.
- **Phase 1**: frames live in Extended Memory (XM) rather than a
  reserved register range — the deliberate tradeoff of narrowing to
  XM-equipped hardware (a real HP-41CX, or a base HP-41C/CV plus the
  real HP 82180A module) in exchange for costing the user's main
  register file nothing at all. Default (tunable) recursion depth 8.
- **Phase 2**: `LCLS`/`LCLX` (frame push/pop) work end to end. One
  scope cut from the original design, not yet revisited: frames are
  currently a **fixed width of 4 registers**, not the originally-
  envisioned variable width — every frame costs the same regardless of
  how much a subroutine actually needs.
- **Phase 3**: local-variable read/write within a pushed frame.
  Originally its own `LSTO`/`LRCL` wrapper functions; later retired in
  favor of calling the real `SAVEX`/`GETX` primitives directly, once
  real-hardware testing found the wrappers had hardcoded a CX-only
  address that doesn't exist on other configurations.
- **Phase 4**: `LRST`, a manual recovery function for a frame orphaned
  by an abrupt exit (a FOCAL error abort, a `GTO` out of a subroutine,
  a manual stop) — no automatic detection by design, since reaching
  into HP-41 OS internals has repeatedly been this project's worst
  debugging experience.

**Compatibility testing is done.** Three angles, all passing:
MultiFOCAL's module changes nothing when not invoked; its own XM usage
doesn't corrupt an independent native file; and a literal, hand-keyed
stored FOCAL program (real `STO`/`RCL`/`GTO`/`LBL` keystrokes, entered
via real `PRGM`-mode key sequences) behaves identically whether the
module is present or not.

**The real hardware target — a base HP-41C/CV plus the real HP 82180A
module — is verified working, in emulation.** Every MultiFOCAL function
(`LCLS`, `LCLX`, `LRST`, local-variable access) has been confirmed
correct against a real, user-sourced 82180A ROM image, not just a CX
standing in for it. Actual physical hardware remains untested: the
user owns the real HP-41CV but has no way to load software onto it yet
(no EPROM burner or flash-based module emulator).

**A real, hand-authored demonstration program now exists** — a genuine
stored FOCAL program (two global labels, one calling the other as a
real subroutine) exercising `LCLS`/`LCLX`/`SAVEX`/`GETX` together,
proving two independently-active local frames don't collide. Building
it surfaced and fixed a real bug in the already-tagged Phase 2 MCODE:
`LCLS`/`LCLX`/`LRST` returned control via a mechanism (`golong ERR110`)
that silently corrupted later XM operations whenever called as a step
*inside a running FOCAL program* — invisible until now because every
prior test drove them via live keystrokes only. Fixed by returning via
a plain `rtn` instead; the whole existing test suite, including the
real CV+82180A configuration, was re-verified afterward with zero
regressions.

See `CLAUDE.md` for the full session-by-session history, the reasoning
behind each decision, and what's still open (variable-width frames and
slot-literal ergonomics for local-variable access).

## Building and testing

Requires the [`gh`](https://cli.github.com/) CLI (to fetch the Calypsi
toolchain release) and a sibling checkout of
[`soynut`](https://github.com/jacob-rn-wallace/soynut) at `../soynut`,
including its own "bring your own ROM" HP-41 OS images — see
`toolchain/README.md` and `../soynut/roms/README.md`. The CV+82180A
test suite additionally needs a real HP 82180A module ROM image (see
"What's not included" below).

```
make mod            # assemble/link src/*.s -> a MOD1 file -> a ROM C array
make test           # ...then run the Phase 0 headless emulator loop test
make cx-roms        # convert soynut's XNUT/CXFUNS ROMs for the CX-based test suite
make cv82180a-roms  # convert a real 82180A module ROM for the CV+82180A test suite
```

The full test suite (frame push/pop, local-variable access,
compatibility testing, the CV+82180A verification) lives under `test/`
as individual programs, each built by hand rather than through one
aggregate target — see `CLAUDE.md` for the exact build command for any
specific test.

## Try it interactively

`test/` proves correctness via automated keystroke injection; `sim/`
is the real, hands-on counterpart — a host-native SDL2 window running
the same real Nut CPU core and real ROMs, with the real 82180A module
and MultiFOCAL's own module both wired in, driven by an actual
keyboard instead of a C program:

```
make mod cv82180a-roms   # if not already built (see above)
make -C sim run
```

See `sim/README.md` for controls and exactly how to invoke `LCLS`,
`LCLX`, `LRST`, and local-variable access from the keyboard.

## What's not included

- **No HP-41 ROM firmware.** The genuine OS ROM and any real module
  images this project tests against — including the real HP 82180A
  Extended Functions/Memory module, needed for the CV+82180A-specific
  tests — are HP's copyrighted calculator firmware, not open source,
  and aren't distributed here — see `../soynut/roms/README.md` for how
  to supply your own.
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
