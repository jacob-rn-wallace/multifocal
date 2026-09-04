# MultiFOCAL

**Repo:** https://github.com/jacob-rn-wallace/multifocal (public). Commit and
push regularly at real checkpoints as work progresses — standing preference,
not something to wait to be asked for each time.

Local variable scoping for FOCAL (HP-41 native language), via subroutine-local
storage frames. Scoping only — no typed/structured data (that's the separate,
independently-timed **ConFOCAL**/VariFOCAL project). Full goals, constraints,
and phase plan: see the original kickoff prompt (not checked in here; ask the
user if it needs to be recovered).

**Hard constraint:** stay on original HP-41 hardware as long as possible.
Flag explicitly before any design choice that requires leaving it.

**Compatibility:** native FOCAL programs must behave identically whether or
not MultiFOCAL is present. Additive functions only — no opcode/syntax changes.

**Packaging:** standalone MCODE ROM/module with its own FAT, same model as
PPC ROM / Advantage Pac. Not a fallback — the intended approach.

**Relationship to `~/soynut`:** soynut is a *separate* project (a from-scratch
HP-41 hardware replica running real ROMs on a real Nut CPU emulator on a Pico).
It is NOT MultiFOCAL and its NASA/JPL "Power of 10" coding standard does NOT
apply here unless the user says otherwise. MultiFOCAL reuses soynut's
`emu41gcc`-based host simulator (`soynut/sim/`) as its test harness only —
by user decision (2026-09-04), to avoid rebuilding a working real-ROM-loading
pipeline from scratch. Only the test harness is shared; MultiFOCAL's own
source lives entirely in this repo.

## Phase 0 status (2026-09-04)

**Toolchain confirmed working end-to-end:**

- **Assembler/linker**: Calypsi (`asnut`/`lnnut`/`dbnut`/`modtool`/`nlib`), the
  actively-maintained successor to NutStudio (itself the successor to the
  1990s DOS-only SDK41). Installed locally, no sudo, at
  `toolchain/calypsi-nut-5.18/` — mirrors soynut's own precedent of extracting
  a `.pkg` payload rather than running a system installer. Add to PATH:
  `export PATH="$(pwd)/toolchain/calypsi-nut-5.18/bin:$PATH"`.
  Docs: `toolchain/calypsi-nut-5.18/doc/` (guides for both the assembler and
  `dbnut` debugger). Source: https://github.com/hth313/Calypsi-tool-chains
  (release 5.18, 2026-07-17).
- **Emulator**: soynut's real Nut CPU core (`emu41gcc`) and real ROM images
  (`~/soynut/roms/`), reused read-only via relative include paths from this
  repo's own `test/` harness — nothing under `~/soynut` is built or modified.
- **Full loop confirmed working, end to end.** `src/mftest.s` (MultiFOCAL's
  own throwaway Phase 0 proof-of-concept MCODE, one FAT entry named
  `MFTEST` — not a real function; naming is still open) assembles and links
  via Calypsi (`make mod`) into `build/mftest.mod`, which soynut's
  `roms/mod_to_c.py` converts to `build/mftest_rom.c`. `test/` wires that ROM
  array into page 8 of a headless (no SDL) build against soynut's Nut CPU
  core — `test/nut_rom_multifocal.c`, following the exact pattern of
  soynut's own `nut_rom_hpil.c` for wiring in an optional module — and
  `test/phase0_loop_test.c` boots the real OS, types the real keystroke
  sequence `XEQ ALPHA MFTEST ALPHA` over the same wire protocol
  `hp41_key_bridge.c` implements, and confirms the display reads
  `MFTEST WORKS`: XEQ-by-name genuinely found the module's FAT entry on
  page 8 and ran real MCODE out of it, not that the routine was just
  jumped to directly. `make test` from a clean `build/` reproduces this
  from scratch.
  **Two debugging findings worth remembering** (full explanation in
  `test/phase0_loop_test.c`'s header comment): `executeNUT(n)` returns as
  soon as the display goes dirty, which happens almost every instruction
  during idle LCD refresh, so it nets roughly one real instruction per
  call while idle — drive it via many outer-loop iterations, like soynut's
  own real main loops do, not via a large instruction count in one call.
  And after POWOFF (auto power-off/sleep, which the emulator core doesn't
  undo by itself), the next key must be preceded by `flagKey=0; regPC=0;`
  (soynut's `sim_handle_sleep_state()` does exactly this) or nothing
  further gets processed.

**Corrected assumption**: FAT function names are NOT limited to a fixed
short length (I'd assumed ~7 characters by analogy with global LBL names).
The HelloWorld example uses `.name "HELLO WORLD"` (11 characters) for a FAT
entry without issue — names are LCD-character strings with an end marker,
not fixed-width. Re-confirm the practical/catalog-display limit later if it
matters for `LCLS`/`LCLX`/`LSTO`/`LRCL` naming, but there is no hard
assembler-level cap at 7.

**Community-convention resource found**: Calypsi ships
`module-export/*.modexport` files cataloging FAT/name info for a large set
of real HP-41 modules (PPC ROM, CCD, HEPAX, Extended Functions/IO, Advantage-
family, Card Reader, Printer, HP-IL, Time, Math, Stat, and others — see
`toolchain/calypsi-nut-5.18/module-export/`). This is a ready-made resource
for the "check against popular existing modules" compatibility task, and for
finding de facto naming/FAT conventions, ahead of any need to disassemble
ROMs by hand. `isene.modexport` is Geir Isene's own module — his coding
standard was separately named in the kickoff prompt as a possible community
reference to check.

**Also on hand, relevant to later phases:**
- `~/Downloads/sy41clv5.pdf` — SY41CL (Monte Dalrymple's reprogrammable
  HP-41CL module board) schematics/manual, for the eventual real-hardware
  testing phase.
- Ruled out for MCODE dev: `hp41-cli`/`hp41-gui` (`~/Downloads/hp41-cli-*`)
  is a from-scratch Rust *behavioral* reimplementation (doesn't execute real
  MCODE); `/Applications/my41cx.app` is a closed commercial iOS/Catalyst app
  with a paid module library, not a dev tool. Neither can load a
  hand-assembled MultiFOCAL module.

## Popular-modules survey (2026-09-04)

Grounded in web research (hpmuseum.org forum/community commentary — its
`xroms.htm` XROM registry page itself 403s automated fetches, but its
content was corroborated via search snippets and a mirror) plus a direct
scan of the real FAT/name data Calypsi ships for ~55 modules
(`toolchain/calypsi-nut-5.18/module-export/*.modexport` — see below).
Ranked by genuine community prevalence, not just "exists":

1. **Advantage Pac** — widely regarded as HP's best official Pac (solver,
   integration, matrix, complex, curve-fit, TVM as real functions, not
   RPN routines). XROM 22, 24 per its `.modexport`.
2. **CCD Module** — extremely well-regarded third-party module; much of
   Advantage's matrix functionality was taken from it. XROM 9, 11.
3. **PPC ROM** — historically hugely influential (the community itself,
   "PPC", is named after it), though now mostly of collector/historical
   interest. Famous for packing ~120 functions into 2-character names to
   fit physical key-overlay labels. XROM 10, 20.
4. **Extended Functions/Memory Module** — close to a de facto standard for
   CX-class usage (extended memory + many now-essential utility
   functions). XROM 25.
5. **HEPAX** — widely used memory/utility expansion module. XROM 7.
6. **Card Reader, (Thermal) Printer, HP-IL Module** — official HP
   peripherals with correspondingly wide real-world install base (already
   present as real ROM images in `~/soynut/roms/`). XROM 30; 29; 28/29.
7. **Math Pac, Stat Pac, Time Module** — official, common HP Pacs.
   XROM 1; 2; 26.

**Confirmed community convention, correcting an earlier assumption of
mine:** XROM ID numbers collide constantly in the wild and this is
accepted, not a real compatibility gate. Scanning all 55 `.modexport`
files' declared XROM numbers turned up extensive overlap even among
official HP Pacs (e.g. XROM 1 alone is claimed by Math Pac, Math/Stat Pac,
and two different 41Z modules; XROM 10 by PPC ROM, Games Pac, Autostart,
and Crypto41). HP's own historical guidance was to use XROM 21 or 31 for
custom ROMs — and per the same scan, even those two "recommended" slots
are already claimed by multiple community ROMs. This makes sense once you
remember XROM numbers are an internal addressing detail (`XROM rn,fn`,
shown only as a fallback display when the owning module isn't physically
present) — a user only ever has a few modules plugged in at once, and
`XEQ ALPHA <name> ALPHA` searches by **name**, not by numeric ID. **The
real compatibility gate is function-**name** collision in the catalog
(plus register-range overlap)**, exactly as the original kickoff brief
already guessed — this research confirms that guess rather than
overturning it. I'd picked `.con 30` for `src/mftest.s` without checking
this and it turns out to collide with the real Card Reader module's own
XROM 30 — harmless for a throwaway, disclaimed, never-distributed test
module, but a reminder to actually check the `.modexport` scan (or
equivalent) before picking a real number for anything that ships.

## Community coding-convention research (2026-09-04)

Geir Isene's published HP-41 coding standard
(isene.me/hp-41/coding-standard) is **FOCAL-only** — a label-numbering
scheme (LBL A's local subroutines at 20-24, LBL a's at 25-29, etc.),
header/body/footer program structure, and one general discipline worth
carrying into MultiFOCAL's own design ethic even though it's phrased for
FOCAL: *every routine must return to the header or a local alpha label as
its last step* — i.e., don't leave control flow (or, for MultiFOCAL, a
frame) in a dangling state. Directly relevant to Phase 4's "what happens
to a stuck/orphaned frame on abrupt exit" question. It has **no
MCODE-specific naming/calling-convention guidance** — there's no
community standard being overridden by picking MultiFOCAL's own MCODE
conventions from scratch in Phase 1.

No prior art was found for a MultiFOCAL-equivalent (subroutine-local
storage frames) already existing as a module or proposal in the
community — this appears to be a genuinely new capability for the
platform, not a reinvention of something already tried.

**Phase 0 is now fully complete.** **Not yet started:** all of Phase 1's
open decisions (frame storage location, frame size, recursion ceiling, LCL
naming) — and the joint register-budget conversation with ConFOCAL that's
supposed to happen before Phase 1 commits to anything.
