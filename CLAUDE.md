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

**Real-hardware testing target: HP-41CV** (the user's own unit; noted
2026-09-04, not yet acted on - Phase 3 work hasn't started). This makes
concrete a question Phase 1 had left open ("HP-41CV's XM compatibility
is genuinely unclear... needs direct verification" - see Phase 1's own
section below): a CV has no built-in Extended Memory, so real XM
support would come from a plug-in module (the period-correct HP 82180A
Extended Functions/Extended Memory module, per Phase 1's own research),
not the fixed mainframe-ROM addresses a CX provides. All Phase 0-2 work
so far has been tested exclusively against soynut's CX boot config
(`nut_boot_cx()`, `XNUT0-2.ROM` + `CXFUNS0-1.ROM`) - genuinely untested
against a CV+82180A configuration. One relevant data point already on
hand, not yet verified as sufficient: as of the `phase-2` tag,
`src/frames.s`'s actual LCLS/LCLX code doesn't call any CX-specific
fixed mainframe address at all (RESZFL/CRFLD calls were removed from
the hot path this phase - see Phase 2's own sections below) - only
`ERR110` (base `mainframe.h`, universal) and its own self-contained
`gsbp` helpers. Its `#include "mainframe_cx.h"` is consequently unused
dead weight right now, left in from an earlier iteration - a candidate
for cleanup, not yet acted on. This is a reason for optimism, not a
verified conclusion: the one-time `CRFLD` setup itself (done via real
keystrokes in the test harness, i.e. name-based catalog dispatch, not a
hardcoded address) should in principle work the same way against a real
82180A module, but this has never actually been tried against a
CV-shaped ROM boot.

**Scope decision (2026-09-04, discussion only - no code changed):** a
CV (the user's actual unit, unmodified) is the near-term target;
"upgrading" it via a CX-class module or an HP-41CL-family board (e.g.
SY41CL, whose manual is already on hand per the note above) to gain
real XM is an explicit stretch goal, not something to design around
now. Considered and set aside in the same discussion, as premature
scope expansion: also supporting a bare 41C brought up to CV-equivalent
register count via a memory module.

**The XM-hinges-everything tension above is resolved, not left open**:
the user confirmed the storage design should keep depending on XM (the
register-range alternative Phase 1 rejected is NOT being revisited) -
and pointed out the real path this project's own Phase 2 groundwork had
already found without connecting it: on a base 41C/CV, XM's own support
routines (`GETX`/`SAVEX`/`CRFLD`/`RESZFL`) live inside the 82180A *port
module*, dispatched by name through the catalog - unlike the CX's other
extras, XM was architecturally always a port-pluggable capability, not
baked into internal mainframe ROM. A modern flash-based port-module
emulator (the user named **TULIP4041**) plausibly emulating that
82180A behavior would need no internal/CPU-board modification at all -
and this project's own MCODE is already positioned for it, since the
one-time `CRFLD` setup dispatches by name, not a hardcoded address
(see the `phase-2`-tag note above). Expected real deployment shape,
per the user: MultiFOCAL's own module and an emulated 82180A would
likely sit side by side as separate flash-hosted ROM images on the
same device - consistent with this file's own "Packaging: standalone
MCODE ROM/module" decision. **Confidence caveat**: this is inference,
not a verified fact - nobody has confirmed TULIP4041 specifically
emulates 82180A/XM behavior at the ROM/port level, only that XM's
architecture makes this plausible in principle. Verify directly before
relying on it once real-hardware testing actually starts.

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

**Phase 0 is fully complete.**

## Phase 1 decisions (2026-09-04)

**1. Frame storage location: Extended Memory (XM), not a reserved register
range.** Explicit, deliberate hardware-scope tradeoff, made with the user's
sign-off after seeing the numbers:

- A reserved register range works on *any* HP-41 down to a bare 41C (63
  registers total), but even a small reservation (~8-12 registers) is a
  painful ~15-20% bite out of the smallest machine's *entire* memory.
- XM costs the main register pool nothing — it's a separate ~100-200+
  register pool organized as named files, not shared with the 63-319
  register main file at all.
- **Corrected an assumption made when this tradeoff was first framed**:
  XM is *not* CX-exclusive. The real, period-correct HP 82180A "Extended
  Functions/Extended Memory" module gives a base HP-41C genuine XM
  capability too - so choosing XM as primary narrows things less than
  "CX-only" would have. **HP-41CV's XM compatibility is genuinely unclear
  from what was researched** (contradictory claims found - CV's built-in
  319 registers may or may not conflict with XM's address space) and
  needs direct verification before stating a firm answer; don't assume
  either way yet. **This is no longer a hypothetical** - see the
  "Real-hardware testing target: HP-41CV" note near the top of this
  file (2026-09-04): the CV is specifically the user's own unit and the
  actual real-hardware target, so this verification is now a concrete,
  load-bearing Phase 3+ task, not just due diligence.
- This is a hardware-*scope* narrowing (which real HP-41 configurations
  are supported), not a violation of the "stay on original hardware"
  constraint itself (XM is 100% original, real HP hardware) - worth being
  precise about the distinction.
- XM is explicitly a shared resource with ConFOCAL's future needs (see
  below), which register-range storage would not have been to the same
  degree - this makes the still-pending ConFOCAL coordination more
  load-bearing under this design than it would have been otherwise.

**2. Frame size: variable-width**, reversing an initial fixed-width-first
recommendation once the storage location moved to XM. XM already provides
file-based, variable-length allocation via the OS's own routines
(`SAVED`/`GETD`/`EMROOM`-style) - the "meaningfully more complexity, pulls
Phase 3+ work forward" cost the original brief flagged for variable-width
was specific to hand-rolling an allocator inside the flat register file,
and mostly doesn't apply once XM's own primitives are doing that work.

**3. Recursion depth ceiling: default 8**, as a tunable assembly-time
constant (not hardcoded) - deeper than the Nut hardware's own native
4-level call stack (so MultiFOCAL is never more restrictive than existing
native nesting for typical use), while still leaving the large majority of
a typical XM pool free, since that pool is shared with ConFOCAL.

**4. LCLS/LCLX/LSTO/LRCL naming**: still genuinely open (not gating
Phase 2's frame-stack work) - to be resolved in Phase 3, checked against
the `.modexport` name-collision data from the popular-modules survey above
before anything is finalized.

**ConFOCAL register/XM budget**: still not a real negotiation (ConFOCAL
doesn't exist as a project yet) - proceeding on an explicit placeholder
basis per the user's direction (2026-09-04): keep the default XM
reservation small and clearly documented as provisional, and re-confirm it
once ConFOCAL exists with its own real numbers. This is now more
load-bearing than it would have been under a register-range design, since
XM is the literal shared resource both projects would draw from.

## Phase 2 groundwork: XM calling convention verified (2026-09-04)

Before writing any frame-stack MCODE, did the research Phase 1 flagged as
necessary: how does third-party MCODE actually read/write registers in an
Extended Memory file? Two-step process, both grounded in primary sources:

**Step 1 - FOCAL-level contract**, from the real HP-41CX Owner's Manual
Volume 2, Section 13 ("Extended Memory," pp. 209-227):
- XM is a **stateful cursor, not random access by index**. `SEEKPTA`
  (ALPHA=filename, X=position) selects a file as "current" and positions
  its pointer in one call; `GETR`/`GETRX` then read from wherever that
  pointer sits and auto-advance it; `SAVER`/`SAVERX` write the same way.
  `RCLPTA` reads the current filename+pointer back out.
- `CRFLD` (ALPHA=filename, X=register count) creates/opens a file.
  `RESZFL` (X=new count) resizes the current file in place.
- **Consequence for frame push/pop**: "current file + pointer" is global
  OS state, not scoped to MultiFOCAL's own calls. Every frame operation
  must save the caller's file/pointer context (`RCLPTA`) before touching
  XM and restore it after, or it will silently corrupt whatever XM file
  the user's own program (or, later, ConFOCAL) had open.

**Step 2 - Nut-level calling convention**, confirmed by directly
disassembling the real `CXFUNS0.ROM`/`CXFUNS1.ROM` pages (`tools/`,
gitignored ROM-derived output, `make -C tools cxfuns cx_disasm` to
reproduce): `GETRX`/`SAVERX` share one code body gated by a mode flag
(ST bit 7) and read their numeric parameter via `GTINDX` - the same
utility an ordinary FOCAL `XEQ` uses to read the visible **X register**.
`SEEKPT`/`SEEKPTA` and `CRFLD`/`CRFLAS` show the identical pattern (a mode
flag selecting between paired variants) and read filenames via
`GTFLNA`/`FLSHAP`-family utilities against the **ALPHA register**. There
is no special raw-Nut-register calling convention to reverse-engineer:
**these routines expect the same FOCAL-level stack/ALPHA state a real
keystroke sequence would have already set up**, and calling them from
MultiFOCAL's own MCODE means constructing that state first, then
dispatching in - with all the same visible side effects (stack lift, etc.)
a real XEQ would have. This upgrades the confidence on this point from the
initial research pass's "medium-low, unconfirmed inference" to "high,
directly verified against real ROM code."

**Also confirmed, a real correction to how the CX-vs-82180A dual-hardware
target has to be implemented**: `mainframe_cx.h`'s addresses (e.g. `GETRX`
at `0x3E36`) are fixed only because they live in the CX's built-in
mainframe ROM. On a base HP-41C + the 82180A module, the identical
functions live in a **plug-in module at a port-dependent address** - a
hardcoded `GOSUB` to `mainframe_cx.h`'s address (the same technique
`HelloWorld.s` correctly uses for base-OS calls, since those *are* fixed
on every variant) would be silently wrong on that configuration. The fix
is the same "ordinary cross-module call" mechanism the original kickoff
brief already anticipated: dispatch by name/XROM number through the OS's
own catalog lookup rather than a fixed address. XROM 25 (Extended
Functions/Memory) is the target, per the popular-modules survey's
`.modexport` scan above.

## Phase 2: XM register read/write verified, with a correction (2026-09-04)

Extending Phase 2 groundwork to an actual working read/write cycle
surfaced a real mistake in the groundwork above, now fixed, plus a
genuine emulator bug worth documenting permanently.

**Correction to the "Phase 2 groundwork" section above**: the claim there
that `SAVERX`/`GETRX`'s internals were "confirmed via disassembly" was
built on disassembling the *wrong* ROM - `tools/cx_disasm.c` was wired
with soynut's plain `NUT1.ROM` (the C/CV base OS) at page 1, not
`XNUT1.ROM` (the real CX base OS). **`NUT0-2.ROM` and `XNUT0-2.ROM` are
genuinely different ROM dumps** (confirmed with `cmp`) - the plain
variant's OS never looks for CX extension pages at all, so combining it
with `CXFUNS0-1.ROM` is not a valid hardware configuration (mirrors real
hardware: a genuine CX's mainframe ROM is one built-together 5-page unit,
not a C/CV with extra pages bolted on). The overall calling-convention
*model* (FOCAL-level parameter passing via `GTINDX`/`GTFLNA`, no exotic
raw-register convention) turned out to still be correct, but the specific
address-level claims about `SAVERX`'s internal control flow in that
section were analyzing bytes that don't actually run in a real CX boot -
treat them as retracted. The right boot config is `XNUT0/1/2.ROM` at
pages 0-2 + `CXFUNS0/1.ROM` at pages 3-4 (`test/nut_rom_cx.c`/`.h`,
`nut_boot_cx()`), confirmed against the emulator's own save-state format
in `emu41gcc/loader.c`: `espaceRAM` registers 64-191 are the CX's built-in
128 XM registers.

**Real mistake in the primitives themselves, also corrected**: `SAVER`/
`GETR` are **bulk whole-file copy operations** (copy every main data
register into/out of the file at once), not single-register read/write -
a wrong assumption in both the original research fork and my own
follow-up. On a fresh cold boot there are zero sized data registers, so
`SAVER` correctly copies zero registers and "succeeds" with no visible
effect - which is exactly the confusing "ran with no error, nothing
changed" symptom that triggered a long debugging detour. **The real
single-register primitives are `SAVEX`/`GETX`** (not `GETXX` - that
appears in `mainframe_cx.h` but isn't a real catalog name; typing it
returns `NONEXISTENT`). `SAVEX` writes X into the current file's current
register and advances the pointer; `GETX` reads the current register into
X. **Proven empirically** in `test/xm_probe_test.c` (`make -C test
xm_probe`) via direct `espaceRAM` inspection, not just the display: a real
write lands at the expected registers, confirmed by a byte-level diff
before/after `SAVEX`.

**A crash that looked like an `emu41gcc` bug, but wasn't - root-caused and
fixed in our own test harness.** `SEEKPT`/`SEEKPTA` (and, separately,
`SAVEP`/`EMROOM`) initially appeared to reliably crash the emulator
(AddressSanitizer showed a wild pointer read in `fetch1()`). The real
cause: `test/nut_rom_cx.c` had `CXFUNS1.ROM` wired at a flat page 4. Real
HP-41CX hardware doesn't work that way - pages 0-3 are flat ROM
(`XNUT0-2`, `CXFUN0`), but **page 5 is bank-switched between `TIMER`
(bank 0) and `CXFUN1`/`CXFUNS1.ROM` (bank 1); page 4 isn't used at all**
(confirmed against a real HP-41CX emulator's own published page-mapping
config). `emu41gcc/nutcpu.c` already has a generic bank-switch mechanism
for exactly this (`typmod[page]==2`, the "CXTIME" case in `enrom()`) -
`nut_rom_cx.c` just wasn't using it. `CRFLD`/`SAVEX`/`GETX` happened to
work under the old, wrong wiring because they never need code actually
located on page 5; anything that does (apparently including `RESZFL`,
found crashing the same way once tested) hit the missing page and
crashed. **Fixed and empirically re-verified** (`test/resz_probe_test.c`,
`make -C test resz_probe`): with page 5 correctly bank-switched, `RESZFL`
grows/shrinks cleanly (a shrink past valid bounds now produces a real,
documented HP-41 error, `FL SIZE ERR`, instead of a crash - exactly the
kind of clean failure mode genuine hardware fidelity should produce), and
`SEEKPTA` seeks to an arbitrary register with no crash at all.

**Consequence**: the earlier "must stay sequential-only, no seeking"
design constraint is lifted. A single growing/shrinking XM file with real
seek-based navigation (the original, more natural variable-width design
from Phase 1/2 groundwork) is back on the table for both Phase 2's frame
push/pop and Phase 3's random-access `LRCL`/`LSTO`.

`emu41gcc` itself was never modified - the bug was entirely in
MultiFOCAL's own `test/nut_rom_cx.c`, reading soynut's real ROMs/core
read-only, per the established convention.

## Phase 2: LCLS/LCLX MCODE - substantial progress, not yet fully working (2026-09-04)

Wrote real `LCLS`/`LCLX` MCODE (`src/frames.s`) and a headless test
(`test/frames_test.c`) exercising 3 nested pushes then 3 pops. Along the
way, found and fixed several real, fundamental MCODE bugs - each is a
durable lesson for all future MCODE in this project, independent of
whether this specific test passes yet:

1. **Page-relocatable calls.** Plain `gosub <local_label>` compiles to an
   *absolute* address under `(position independent)` linking, assuming
   the module loads at whatever page the linker happens to pick - wrong
   whenever loaded elsewhere (page 8, in every test here). The fix is
   Calypsi's dedicated `gsbp`/`golp` instructions (see the Calypsi Nut
   Guide's "Page relocatable jumps" section) for any call to a routine in
   *this module's own page*; plain `gosub`/`golong` stay correct for
   fixed mainframe/CX addresses. Confirmed via instruction trace: the
   first version of this code, without `gsbp`, jumped straight into
   unrelated base-OS code on its very first internal call.
2. **`gsbp` clobbers C.** The page-relocation mechanism itself uses C to
   compute the jump target, so a value placed in C immediately before a
   `gsbp` call is gone by the time the callee's first instruction runs.
   `B` is untouched by it (confirmed empirically) - values now get
   relayed into B via the one-way `a=c m; b=a m` chain before any `gsbp`
   call that needs them.
3. **Field X (assembler) is not the FOCAL X-register's value - it's the
   exponent.** The real 14-nibble HP-41 register format is: nibble 13 =
   sign, nibbles 3-12 = "M" field = the 10 mantissa digits (ones digit at
   nibble 12), nibbles 0-2 = "X" field = the exponent. Arithmetic on the
   *value* (increment/decrement/copy) needs field M and nibble 12, not
   field X and nibble 0 - a name collision between "assembler field X"
   and "the calculator's X-register" cost real time here. Field X
   (nibbles 0-2) remains the correct, deliberate choice for `DADD=C`'s
   address argument specifically - that part was never wrong.
4. **`SEEKPTA` to a file's own last register fails** ("END OF FL"),
   confirmed via pure keystrokes independent of any MCODE (works fine on
   a 10-register file seeking to register 1; fails on a 1-register file
   seeking to its only register 1). Since this design's header lives
   permanently at register 1, the stack file must never be sized such
   that register 1 is also the last register - fixed by creating it at
   size 2 initially rather than 1.
5. Also confirmed, separately: `SAVER`/`GETR`/`SAVEP`/`EMROOM`/`RESZFL`
   crash theories from earlier sessions were self-inflicted bugs (wrong
   ROM in disassembly; a page-5 bank-switch wiring bug in the test
   harness), not real emulator or ROM problems - see the sections above.
   That discipline (assume MultiFOCAL's own code first) held up again
   here and is worth continuing to apply.

**Still unresolved, honestly**: even with all of the above fixed, the
test does not yet show correct values (`SAVEX` at the end of each
`LCLS`/`LCLX` still displays "1.0000" instead of the expected size, and
querying the header afterward reads back 0 every time) - no crashes, no
error messages, just wrong values, meaning something in the register-
passing chain between the `GETX`/arithmetic/`SAVEX` steps still isn't
correct. A hypothesis that this was Nut's 4-level hardware call-stack
being exceeded by this code's helper-routine nesting was tested (inlined
one layer of nesting) and did **not** fix it, ruling that out as the sole
cause. Root cause not yet found - this needs another focused debugging
pass, not a continuation of guessing.

**Scope cuts accepted for this proof-of-concept** (documented in
`src/frames.s` itself too): both `LCLS`/`LCLX` use a fixed width of 4
registers per frame rather than reading a caller-supplied count or being
truly self-describing via a trailer register - the original variable-
width design needs two independent values alive across OS calls
simultaneously, and only one safe carrier register (B) was confirmed;
solvable, but deferred. File creation is a separate one-time setup step
the test harness does, not something `LCLS` does idempotently itself
(calling `CRFLD` unconditionally hits `DUP FL` on repeat calls, and a
real HP-41 error return does not hand control back mid-routine).

**Not yet done (as of first pause):** root-causing the remaining value-
passing bug.

## Phase 2: LCLS/LCLX MCODE, continued - real architectural finding, redesigned around it (2026-09-04)

Picked back up on the value-passing bug with a battery of small, isolated
probe modules (each doing "gosub `<function>`" then a message-display
side effect that only appears if control genuinely returns - not checked
in, throwaway, reproducible from the description below). This found the
actual root cause, which was bigger than a simple bug:

**Confirmed architectural fact: any real X-Function that touches the
ALPHA register abandons its caller and jumps straight to the OS idle
loop when called directly via `gosub` from third-party MCODE, instead of
returning via `rtn` like an ordinary subroutine.** Confirmed for
`SEEKPTA`, `RCLPTA`, and `CRFLD` - each one, called via `gosub` with
correct ALPHA/X set up beforehand, left the *next* instruction (a message
display, chosen specifically because it's only visible if control truly
returns) never executed; the hardware return-address stack (`retstk[4]`
in `nutcpu.h`, confirmed by instrumenting it directly) shows the pushed
return address sitting unclaimed while execution wanders through several
properly-paired `gosub`/`rtn` calls before eventually falling into the
idle address. **Purely numeric functions (`GETX`, `SAVEX`, `RESZFL`) all
confirmed to return normally** via the same technique. This makes sense
architecturally: these are catalog-dispatch-only entry points whose
"success" path is inherently "hand control back to the OS's normal
continuation" (idle for a keystroke, next program step for a running
FOCAL program) - not a case of Nut's 4-level call-stack limit being
exceeded (that hypothesis, tested earlier, is now understood to have been
the wrong tree entirely).

**Consequence - LCLS/LCLX redesigned again, more simply than before**:
since file creation (`CRFLD`) already had to be a one-time keystroke-
driven setup step (not inside `LCLS`), and `SEEKPTA`/`RCLPTA` turn out to
be equally unusable from MCODE, the design drops the header-register/
seek-back approach entirely. `LCLS`/`LCLX` are now pure functions over
the FOCAL X-register: the caller supplies the current stack size in X,
each call computes size±4 and calls only `RESZFL` (relying on "current
file" being persistent OS state that survives across separate `XEQ`
invocations, not just within one call - a real assumption, not yet
independently verified but consistent with the design working so far),
returning the new size in X. No seeking, no ALPHA touch inside
`LCLS`/`LCLX` at all.

**A second real bug found and fixed along the way**: ending `LCLS`/
`LCLX` with a bare `rtn` left the display blank - `regPC` settled at the
normal idle address, so `rtn` did work, but nothing refreshed the
display with X's new value. Every one of this session's *working*
examples (the original `mftest.s`, and all the probe modules above)
ended their top-level completion with `golong ERR110`, never a bare
`rtn` - switching to that fixed it (confirmed: input value now correctly
echoes back after `golong ERR110`, e.g. typing `2` and XEQing `LCLS`
correctly redisplays `2` - the *arithmetic* result is still wrong, see
below, but the display-refresh mechanism itself is now right).

**Current, still-open bug**: the `+4`/`-4` arithmetic itself doesn't
take effect - `LCLS(2)` and `LCLS(6)` currently just echo back their
input unchanged (`2`->`2`, `6`->`6`) instead of `2`->`6`, `6`->`10`.
Traced this directly: a keystroke-typed value (e.g. "2") reads back via
this project's own raw `C=DATA` register read (`LoadXIntoC`) with the
digit sitting at **nibble 12**, but `nutcpu.c`'s own `C=C+1 M`
implementation (confirmed by reading the C source directly, the `case
17` handler) applies the `+1` and carries starting at **nibble 3** (field
M's own `p1`), working upward toward nibble 12. These two positions
don't match, which is why the increment silently misses the real value
entirely (it's incrementing an unrelated, currently-zero nibble).
Working theory, not yet implemented: keystroke digit entry leaves the
value in a "raw"/left-justified form at nibble 12 that only gets
normalized into the standard right-justified BCD mantissa (ones digit at
nibble 3, matching field M's own convention) when read through the OS's
own `GTINDX` utility (the same one `CRFLD`/`SEEKPTA`/etc. use internally
to parse their own X argument, confirmed via disassembly much earlier in
Phase 2 groundwork) - `LoadXIntoC`'s raw read bypasses that normalization
entirely. `GTINDX` is a base-OS *internal* utility (from `mainframe.h`,
not a catalog-visible X-Function), so it should be safe to `gosub`
directly without the ALPHA-abandonment problem above - this needs
verifying, then wiring in, before the arithmetic will work.

## Phase 2: LCLS/LCLX MCODE, continued again - the real number-format split (2026-09-04)

Verified the `GTINDX` theory directly, two ways:

1. **Disassembled `GTINDX` itself** (0x34A4, `#include "mainframe.h"` -
   needs a manual `.equlab`, it's not exported by name): confirmed it
   reads X via `C=REGN 3 (X)` - a *different* addressing mechanism
   entirely from the `DADD=C`/`DATA=C` pair this project's own code has
   been using (`REGN` addresses the small set of fixed FOCAL stack/flag
   registers directly by index; `DADD`/`DATA` address the general
   numbered main-memory register file - `DADD=3` happens to coincide
   with the real X register too, which is why `StoreBIntoX`/`LoadXIntoC`
   were never *wrong*, just working with a different number
   *representation* than expected). `GTINDX` does real validation/
   parsing (bounds checks, `ST=1 2/3` flag tests) and leaves its result
   in **register N**, confirmed via `N=C` appearing three times.
2. **Empirically confirmed what N actually holds**, with a tiny probe:
   typed `2`, ran `gosub GTINDX; gosub <write N into X>; golong ERR110`,
   and the display showed `0.0002` - not `2`. This is the real, direct
   evidence for the actual split: **the *displayed* calculator number
   format is a normalized float** (value = 0.d1d2...d10 x 10^exp, d1 at
   nibble 12, exponent in nibbles 0-2 - so a typed "2" is really
   "0.2 x 10^1", d1=2 at nibble 12, exponent=1) - **`GTINDX`'s output in
   N is a *plain BCD integer* instead** (ones digit at nibble 3, no
   exponent, no normalization) - matching field M's own `p1=3`
   increment convention exactly, which is *why* `C=C+1 M` "worked" on
   the wrong nibble all along: it was never wrong about field M's own
   layout, the mismatch was calling it on a value still in the *other*
   (float-display) representation.

**Consequence, and the concrete remaining gap**: `GTINDX` is exactly the
right tool to turn a caller's typed X value into the plain-integer form
`C=C+1 M`/`C=C-1 M` can correctly operate on. What's still missing is
the *reverse* conversion - turning the resulting plain integer back into
the normalized float format needed for the final "write X so it displays
correctly" step (plain `SAVEX`/`StoreBIntoX` after arithmetic in the
plain-integer domain will show something like `0.0006` instead of `6`,
the same way the N-into-X probe did). Need to find the base-OS utility
that does this (the natural counterpart to `GTINDX` - likely something
in the "PUTX"/"STOX"/normalize-and-store family, not yet identified) and
verify it the same way: an isolated probe, typed input, `gosub
<candidate>`, check the actual displayed number rather than trusting
that it "ran with no error."

Not yet done: finding and verifying that conversion; then a full,
correct nested-frame test.

## Phase 2: LCLS/LCLX MCODE, continued yet again - GTINDX retracted, self-contained conversion works, new hang found (2026-09-04)

**Retraction**: the `GTINDX`-based plan above does not work. An isolated
probe (`gosub GTINDX` with a known X value, then dump register N's raw
nibbles directly - not just the display) showed N's content does not
correlate with X's actual value at all between X=2 and X=14 - most
nibbles were identical or changed in ways that don't match either input.
Whatever `GTINDX` actually does, it is not the float-to-plain-integer
converter the previous section assumed, and the "confirmed via
disassembly" claim for its *behavior* (as opposed to its calling
convention, which was correctly established back in Phase 2 groundwork)
is retracted.

**What actually pinned down the real number format**: typing digits and
reading the display mid-entry turned out to be misleading (an earlier
probe skipped straight from `type_str` to a raw register dump without
ever pressing a real terminating key, and got byte patterns that don't
match settled values). The reliable method is typing digits *followed by
a real ENTER^ key press* (byte 13 into the key bridge - `tabcode[13]` is
`0x13`, the same code as the `^` character, confirmed in
`hp41_key_bridge.c`) before dumping. Doing this for several values gives
a clean, consistent picture: "1"/"2"/"6"/"9" all settle with mantissa
digit 1 (d1) at nibble 12 and all exponent nibbles (0-2) zero; "10"/"14"/
"30" all settle with d1/d2 at nibbles 12/11 and exponent nibble 0 = 1.
This exactly matches the standard normalized-float model (mantissa
d1.d2...d10 x 10^exponent) and - cross-checked against `nutcpu.c`'s own
`RCR n` implementation, `new_C[i] = old_C[(n+i) mod 14]` - is achieved
from the plain-integer form (ones digit at nibble 3, matching field M's
own carry convention) by a **fixed rotation**: `RCR 5` for a 1-digit
result, `RCR 6` plus explicitly setting exponent nibble 0 to 1 for a
2-digit one. The exact inverses (`RCR 9`, `RCR 8`) convert the other
way. `src/frames.s`'s `LoadPlainFromX`/`StoreFloatIntoX` now do this
conversion entirely themselves - no OS utility dependency, no more
"still missing" gap on this specific point.

**A second real bug found in the same pass: raw C=C+1/-1 M arithmetic is
hex, not decimal, unless told otherwise.** `nutcpu.c`'s add/subtract/
increment/decrement family all gate their carry threshold on a global
`flagdec` ("if (flagdec) m=10; else m=16"), and it defaults to hex
(0) - so incrementing nibble 3 from 9 gave `0xA`, not a BCD-corrected 0
with carry into nibble 4, until `setdec` (real Nut mnemonic, confirmed
in `nutcpu.c`) is issued first. Also found: **leaving decimal mode on
across a subsequent `gsbp` call corrupts its jump target** - `gsbp`'s
page-relocation mechanism is implemented as a mainframe-routine call
that does its own address arithmetic, apparently in hex, so `sethex`
must run before any `gsbp`/`gosub` that follows the decimal-mode
arithmetic. Both fixes are in `frames.s` now: `setdec` right after
reading the plain-integer value, `sethex` right after the arithmetic and
before the second `gsbp` call.

**With both of those fixed, the first LCLS call in a sequence now works
correctly end-to-end**, confirmed both by `test/frames_test.c` (X=2 ->
XEQ LCLS -> display correctly reads "6.0000") and by direct register
trace (X's raw nibbles after the call exactly match a keystroke-settled
"6"). This is real, verified progress on the original number-format
gap - not yet a full pass of the milestone test, but the specific value-
representation problem this whole investigation was chasing is now
resolved.

**Still unresolved, and newly discovered - a hang on the *second*
consecutive LCLS call in a session.** `test/frames_test.c`'s step 2
(X=6 -> expect 10, right after a real, successful step 1) never
completes: the display stays blank even after millions of single-
instruction trace steps (confirmed via an instrumented tracer,
`test/frames_trace_test.c`'s pattern extended to run a real step 1
first). The MCODE itself is not at fault here - traced all the way
through: `LoadPlainFromX` correctly reads 6, the arithmetic correctly
produces the plain-integer 10, `StoreFloatIntoX` correctly writes the
normalized float "10" into X (raw nibbles confirmed matching a real
settled "10"), and `gosub RESZFL` completes with the file visibly grown
to the right register range. The hang happens *after* that, inside
`golong ERR110`'s own display-refresh path (mainframe page 2), which
this module never wrote. The trace shows a genuine tight loop -
`2B14 -> 2B15 -> 2B1E -> 2B1F -> 2B20 -> 2B21 -> 2B22 -> 2B41 -> 2B42 ->
2B5F -> 2B60 -> 2B71 -> 2B72 -> 2B73 -> 2B74 -> 2B75 -> 2B14 -> ...` -
counting down a single hex nibble (via `A=A-1 PT`) by one per full outer
pass, with no sign of reaching zero even after ~300,000 outer passes
(5,000,000+ total instructions). Isolated single-call tests (skip step 1
entirely, jump straight to X=6 -> XEQ LCLS from a size-2 file) do **not**
hit this - the same display/catalog code completes in ~1500 steps and
shows "10.0000" correctly. So the trigger is specifically "a second real
LCLS/RESZFL/ERR110 invocation in the same session," not the 2-digit
value or the "MFSTK" filename by themselves (the isolated case has both
of those too). Compared `flagdec`/`dspon`/`fdsp`/`facces_dsp` at the
moment `ERR110` is entered between the working (isolated) and failing
(sequential) cases - identical in both, so the divergence is something
else, not yet identified, inside that shared OS code path. Whether this
is a genuine second real HP-41 quirk (unlikely - resizing an XM file
twice in a row via plain keystrokes, tested directly with no MCODE
involved at all, works fine both times) or something specific to
re-entering a *custom FAT function* a second time is the open question -
not yet root-caused.

**Not yet done**: root-cause the second-call hang (a good next probe:
trace where the huge/garbage-looking counter value that loop is counting
down actually comes from - work backwards from the `A=A-1 PT` loop at
`0x2B14` to whatever wrote that value into A); then the real milestone
test (nested push/pop sequence, `test/frames_test.c`, all 6 steps
passing).

## Phase 2: second-call hang precisely isolated - likely a real design
constraint, not a fixable MCODE bug (2026-09-04)

Narrowed the hang above to its minimal reproduction, via a battery of
throwaway 2-3 instruction probe modules (not checked in - reproducible
from the description below):

1. **A bare `gosub RESZFL; golong ERR110` (no arithmetic, no number-
   format conversion, none of this project's own logic at all) hangs
   exactly the same way**, called via real `XEQ` twice in a row (grow a
   real CRFLD'd file 2->6, then 6->10). This completely rules out
   `LCLS`/`LCLX`'s own arithmetic/rotation/`SETDEC` logic as the cause -
   the bug lives entirely in `gosub RESZFL` + `golong ERR110` themselves.
2. **The trigger is specifically "`gosub RESZFL` happens a second time in
   the session," not "the same function is XEQ'd twice."** Two
   *different* FAT functions, each just `gosub RESZFL; golong ERR110`,
   XEQ'd back to back (first one growing 2->6, second growing 6->10)
   hang on the second one identically - it doesn't matter which
   function's body issues the second `gosub RESZFL`.
3. **A single `gosub RESZFL` followed by a *different*, unrelated
   function that's just `golong ERR110` (no RESZFL at all) does NOT
   hang** - so one `gosub RESZFL` call doesn't "poison" `ERR110` for
   later, unrelated callers. It specifically takes a *second*
   `gosub RESZFL` in the session before the hang appears.
4. **`gosub ERRSUB` before `gosub RESZFL`** (matching the real
   `HelloWorld.s` example's own opening call, on the theory that it
   resets some status/error-flag state real X-Function dispatch would
   otherwise set up) **does not fix it** - tested directly in this exact
   minimal reproduction.
5. **Replacing `golong ERR110` with a bare `rtn` makes the hang go away
   entirely** (both calls complete immediately, display blank as
   expected without a refresh) - this pins the actual infinite loop
   inside `ERR110`'s own code (confirmed by direct trace: `RESZFL`
   itself always completes and returns normally, correctly growing the
   file to the right size, both times), not inside `RESZFL`, and not a
   stack-depth/overflow effect of calling it via `gosub` (a
   `retstk[3]`-eviction watch confirmed real evictions of stale,
   already-unwound entries happen on *both* the succeeding first call
   and the succeeding "single-resize-then-unrelated-function" case
   too, so eviction alone isn't sufficient to explain the hang either).
6. Cross-checked against the earlier "MFSTK spelled out character by
   character" trace observation (Section "LCLS/LCLX MCODE, continued
   again"): `ERR110`'s display-refresh path does genuine, separate work
   related to the *currently-open XM file's name*, not just the bare X
   value - consistent with the hang being in that file-name-aware
   sub-path, triggered only once a *second* raw-`gosub`-entered `RESZFL`
   call has happened.

**Working theory** (not fully confirmed at the individual-instruction
level, but consistent with every test above): real keystroke/catalog
`XEQ` dispatch into `RESZFL` does some setup or teardown step - beyond
what `ERRSUB` covers - that a raw `gosub RESZFL` from custom MCODE
skips, and this only becomes an observable bug the *second* time it's
skipped in a session, when `ERR110`'s file-name-aware display path later
reads whatever was left inconsistent. Confirmed NOT the cause: `LCLS`/
`LCLX`'s own arithmetic, `SETDEC`/`SETHEX`, `ERRSUB`, or hardware call-
stack depth (real evictions happen on working calls too).

**Real design consequence, if this holds**: `RESZFL` (or possibly any
X-Function with comparable internal complexity) may not be safely
callable via raw `gosub` from custom MCODE more than once per session -
directly threatening the current `LCLS`/`LCLX` design, which needs to
call it on *every* frame push/pop. The likely fix is not a small MCODE
patch but a **design change**: allocate `MFSTK` once, at its full
maximum size (recursion-depth-ceiling x frame-width + header, e.g. 8x4+2
= 34 registers, matching Phase 1's already-chosen depth ceiling), via a
single one-time `CRFLD`/`RESZFL` call done the *same* way file creation
already is (real keystroke/XEQ, not gosub'd from MCODE) - and have
`LCLS`/`LCLX` track "how many registers are actually in use" via their
own counter (still returned/consumed through X, same calling
convention) instead of ever calling `RESZFL` again after that one-time
setup. This trades a small amount of always-reserved XM space (bounded
by the depth ceiling either way per Phase 1's own reasoning) for
avoiding `RESZFL` entirely from within running MCODE - not a hardware-
scope change, not a compatibility change, purely an internal allocation
strategy change.

**Not yet done**: implement and verify the fixed-max-size, no-repeat-
RESZFL redesign; if it avoids the hang, that's strong confirmation of
the theory above without needing to fully disassemble `ERR110`'s
file-name display sub-path to find the exact missing setup step.

## Phase 2 milestone reached: full nested LCLS/LCLX push/pop sequence
passes, plus a second, unrelated real bug found and fixed (2026-09-04)

Implemented the fixed-max-size redesign proposed above: `LCLS`/`LCLX`
no longer call `RESZFL` at all - they're pure counter arithmetic over
X (read/converted/incremented-or-decremented/converted-back/written,
exactly as before, just with the `gosub RESZFL` line deleted).
`test/frames_test.c`'s one-time setup now creates `MFSTK` at its full
maximum size (34 = depth-ceiling 8 x frame-width 4 + header 2) instead
of size 2. This alone made all 3 `LCLS` calls in the milestone test
pass, confirming the RESZFL/`ERR110` hang theory from the previous
section - avoiding `RESZFL` from MCODE entirely does avoid it.

**A second, completely unrelated real bug surfaced immediately after**:
`LCLX` (the decrement direction) never dispatched at all - not a wrong
answer, a real *no-op*: `XEQ LCLX` left X completely unchanged, with
no error message, from a fresh boot or after setup, regardless of X's
value. Traced all the way down (a dedicated PC-watch confirmed `regPC`
never once touches page 8 - LCLX's own page - during the entire
key-processing budget, even though `modtool --summary` shows its FAT
entry as syntactically correct and correctly addressed).

**Root cause, confirmed by direct experiment**: **the *last* `.fat`
entry in a module never dispatches via `XEQ ALPHA <name> ALPHA`**, full
stop - independent of which real function happens to occupy that slot.
Proved by adding a trivial third dummy FAT entry (`golong ERR110`)
after `LCLX` in the `.fat` list: `LCLX` immediately started dispatching
correctly (confirmed via `modtool --summary` showing the new entry's
address, and by direct trace showing execution now genuinely reaching
page 8). This is a real, previously-undiscovered Calypsi/FAT-structure
finding, not an MCODE logic bug - `LCLS` happened to never hit it only
because it wasn't the last entry in this module's own `.fat` list.

**Fix, now in `src/frames.s` and permanent**: added a fourth, deliberate
no-op FAT entry (`.name "MFPAD"`, `Padding: rtn`) as the last `.fat`
entry, so no real callable function is ever last. Documented at both
the FAT table and the `Padding` label itself as a permanent placeholder,
not dead code to remove later - any future new function added to this
module must go *before* `Padding` in the `.fat` list, or `Padding`
must be moved back to last position again.

**Result: `test/frames_test.c`'s full milestone (3 nested pushes then
3 pops, checking every returned size) now passes end to end** - 2 -> 6
-> 10 -> 14 -> 10 -> 6 -> 2, all six steps correct, confirmed stable
across repeated runs. This closes out the number-format conversion
work and the RESZFL/`ERR110` hang investigation from the sections
above, and is the first time this project's actual frame push/pop
mechanism has been shown working end to end.

**Scope reminder, unchanged from the original design**: this milestone
validates the *fixed-width-4* proof-of-concept scope cut - the
underlying variable-width, self-describing-trailer design from Phase 1
is still deferred, and `MFSTK` is now allocated once at a size
(recursion-depth-ceiling x frame-width + header) that assumes the
fixed-width-4 cut too. Phase 3 (`LSTO`/`LRCL`, real seeking/register
access within the pre-allocated file) and generalizing frame width are
still open.

## Phase 2 closed out: recursion-depth-ceiling enforcement added
(2026-09-04)

Before tagging Phase 2 complete, closed a real gap against Phase 1's
own decision ("recursion depth ceiling: default 8"): nothing previously
stopped `LCLS` from being called past the pre-allocated 34-register
maximum, or `LCLX` from being called past empty (2). Neither is a
crash risk (no `RESZFL` call means no XM operation to fail), but both
were silent correctness gaps - `LCLS` would have kept incrementing X
past 34 forever with no error, `LCLX` past 2 into negative territory
(which this project's own number-format conversion doesn't even
support - see the RCR-rotation derivation above, which only handles
0-34).

**Design choice for the refusal itself**: deliberately silent (X left
unchanged, no error message) rather than raising a real HP-41 named
error. A named error needs to *display text*, and every text-display
mechanism confirmed safe so far in this project goes through routines
this specific new code path hadn't independently verified (`MESSL`
etc., per `HelloWorld.s`) - given how many subtle, hard-to-diagnose
bugs this phase already surfaced in far simpler code, the lower-risk
choice was to reuse only what's already proven (the same `golong
ERR110` completion every other path uses) and skip the message
entirely. A caller can tell a push/pop was refused because X comes
back unchanged instead of shifted by 4 - documented in `frames.s`
itself.

**Implementation**: both `LCLS` and `LCLX` now do a bounds check via
`?A<C M` (confirmed via `nutcpu.c` to be a non-destructive, full
10-nibble comparison, correctly BCD-aware once `SETDEC` is set - the
same instruction family, `subreg`, that the arithmetic itself already
depends on) against a constructed constant (34 for `LCLS`, 6 for
`LCLX` - the smallest value it may still safely act on) *before* doing
any arithmetic, branching around the increment/decrement and the
`StoreFloatIntoX` conversion entirely when refused.

**Verified via a new, dedicated test** (`test/frames_bounds_test.c`,
checked in - not throwaway): `LCLS` at exactly 34 is refused (X stays
34); `LCLX` at exactly 2 is refused (X stays 2); `LCLS` at 30 and
`LCLX` at 6 - one step away from each boundary - both still succeed
normally. All four cases pass, and the original 6-step milestone test
(`test/frames_test.c`) still passes unchanged after this addition.

**Phase 2 is now tagged complete** (`phase-2`). What's *not* covered by
that tag, staying explicitly open for later phases: the compatibility
constraint (native FOCAL programs unaffected by MultiFOCAL's presence)
has not been tested at all this phase; the variable-width design from
Phase 1's own tag is superseded by this phase's fixed-max-allocation
approach, which Phase 3+ may need to revisit if variable frame widths
are ever required; and `LSTO`/`LRCL` (actual local-variable read/write
within a pushed frame) don't exist yet - by original design, that's
Phase 3's job, not a Phase 2 gap.

## Phase 3: LSTO/LRCL - local-variable read/write, milestone reached
(2026-09-04)

Started Phase 3 (`LSTO`/`LRCL`: actual local-variable read/write within
a pushed frame) at the user's direction. Before writing any MCODE, did
the research this phase needed: how do you reach an ARBITRARY register
within the current frame, given Phase 2 already established that
`SEEKPTA` (needed to position the XM file's pointer at a specific
register) cannot be safely `gosub`'d from custom MCODE at all? This
took a long, genuinely necessary empirical detour, documented here in
full because every dead end is itself a real, load-bearing finding for
future work in this area - not padding.

**Fixed a real, long-standing gap in this project's own tooling
first**: `tools/cx_disasm.c` had been silently using soynut's plain
`rom_nut0/1/2` (the C/CV base OS) for pages 0-2 ever since Phase 2's
own "SAVERX/GETRX... was built on disassembling the wrong ROM"
retraction - that retraction was written but the tool itself was never
actually fixed. Now uses `rom_xnut0/1/2` (real `XNUT0-2.ROM`), matching
`test/nut_rom_cx.c`'s own boot config exactly (`tools/Makefile` gained
an `xnut` target alongside `cxfuns`). Any of this project's own prior
disassembly that crossed into pages 0-2 before this fix (none of
Phase 2's *load-bearing* conclusions did - CXFUNS's own pages 3-4 were
always correct) should be treated as unverified until re-checked
against the corrected tool.

**Finding 1 - `SEKPT` (the non-ALPHA sibling of `SEEKPTA`, 0x3f2c in
`mainframe_cx.h`, vs `SEEKPTA`'s 0x3f35) ALSO abandons its caller when
`gosub`'d directly**, exactly like `SEEKPTA`/`RCLPTA`/`CRFLD` do -
confirmed via the same sentinel-write probe technique Phase 2 used
(`gosub SEKPT` then a value-write that only executes if control
actually returns; it never did, for two separate targets in the same
session). This retracts the natural-seeming hope that "ALPHA-touching"
was the precise dividing line between safe and unsafe `gosub` targets -
`SEKPT` touches no ALPHA at all (no filename argument) and still
abandons. The real dividing line, as best understood now: purely
numeric single-register ops (`GETX`/`SAVEX`, confirmed again this
phase) return normally; anything that repositions the file pointer -
ALPHA-based or not - does not.

**Finding 2 - `GETRX`/`SAVERX` (0x3e36/0x3e2f) are NOT a safe
index-based random-access primitive either.** Disassembly (using the
now-fixed tool) showed they share one code body with `GETR`/`SAVER`
(0x3e62/0x3e69, the already-known bulk whole-file copy operations),
selected by the same kind of mode-flag pattern seen elsewhere in this
ROM - `GETRX`/`SAVERX` are just the "count comes from the X register at
runtime" entry variant of the SAME bulk copy, not a different
operation. Confirmed empirically too: `XEQ SAVERX` with a plausible
single argument produced no XM data-register change anywhere in the
full 1024-register `espaceRAM` range (only OS-internal scratch/catalog-
search registers changed) - the exact same "succeeds with no visible
effect" trap Phase 2 already hit once with plain `SAVER`/`GETR` on a
file with nothing sized yet.

**Finding 3 - a second, independent, and more general form of the
Phase 2 "`SEEKPTA` to a file's own last register fails" bug.** Phase 2
had only confirmed this for the degenerate case of a 1-register file
(where register 1 is also the *only* register) and worked around it by
using size 2 for `MFSTK`. Directly tested this phase, via pure
keystrokes with no MCODE involved at all: `SEEKPTA` to register 10 of a
real 10-register file fails with "END OF FL" too - and so does
`SEEKPTA` to register 34 of a real 34-register `MFSTK`. **This
generalizes: `SEEKPTA` to a file's own last register always fails,
regardless of file size** - not a degenerate-size-1 quirk. Consequence:
`MFSTK` must be created at size **35**, not 34 - one permanent, never-
touched padding register past the depth ceiling's own last real slot
(register 34), verified directly to restore normal seeking to register
34 once the file is one register larger than its logically-used range.
This is a real HP-41CX OS limitation, not something this project's
MCODE can work around any other way (no seeking happens from MCODE at
all, so there's no code path here to fix).

**Finding 4 - the real, working design**: since `SEEKPTA` cannot be
called from MCODE at all (Finding 1 covers the only real alternative
and rules it out too), the CALLING FOCAL PROGRAM must do the seek
itself, as an ordinary top-level program step (`"MFSTK"` `<register>`
`XEQ SEEKPTA`), immediately before `XEQ LSTO`/`XEQ LRCL` - exactly the
same pattern already established for `CRFLD` (a real FOCAL/keystroke
step, never `gosub`'d). `LSTO`/`LRCL` themselves are then thin wrappers
around the confirmed-safe single-register primitives: `LRCL` is
`gosub REALGETX; golong ERR110` (`REALGETX` = a locally-declared
`.equlab 0x380B`, disassembly-confirmed to share SAVEX's body and end
in a real `RTN` - `mainframe.h` already declares its OWN, unrelated
`GETX` at 0x1CEF, a coincidental name collision caught by the
assembler's "duplicate symbol" error, not silently wrong). Verified
end to end: pushing all 8 frames to the depth ceiling, storing 4
distinct values into the shallowest frame's registers (3-6) via one
`SEEKPTA` plus 4 sequential auto-advancing `LSTO`s, storing into the
*deepest* frame's registers (31-34 - including the previously-
unreachable register 34) in deliberately scrambled order (a fresh
`SEEKPTA` before each), then reading all 8 values back via `LRCL`
(sequential for the shallow frame, scrambled again for the deep one) -
every value round-tripped correctly (`test/lsto_lrcl_test.c`, checked
in).

**Finding 5 - a real, confirmed bug drives `LSTO`'s one asymmetry with
every other function in this module**: `gosub SAVEX` followed by
`golong ERR110` corrupts the OS's file-directory lookup - the CALLING
FOCAL PROGRAM'S very next real `SEEKPTA` fails with a spurious "FL NOT
FOUND" (the file appears to not exist at all), even though `SAVEX`
itself completes correctly and returns normally every time. Isolated
via a battery of minimal probes, each varying exactly one thing:
  - Real catalog-dispatched `XEQ SAVEX` (no `gosub` at all): no
    corruption, confirmed via a subsequent real `SEEKPTA` + `GETX`
    round-trip.
  - `gosub REALGETX` (the read path) followed by `golong ERR110`: no
    corruption either - the bug is specific to the WRITE path.
  - `gosub SAVEX` followed by a bare `rtn` (no `ERR110` at all): **no
    corruption** - confirmed correct at every register including the
    file's actual last one, verified independently via real `GETX`
    reads afterward. This isolates the bug to the specific combination
    of `gosub SAVEX` immediately followed by `golong ERR110`, not to
    `SAVEX` or `gosub` individually.
  - Two attempted fixes that did NOT work, for the record: clearing ST
    bit 7 (the mode flag `SAVEX`'s own entry sets and never explicitly
    clears before its `RTN`) right after `gosub SAVEX`, and `gosub
    ERRSUB` (the real X-Function "opening housekeeping" call
    `HelloWorld.s` always does, tried once before for the unrelated
    Phase 2 RESZFL hang without success there either) before `gosub
    SAVEX` - both instead broke the *current* call outright (`NONEXISTENT`
    immediately), rather than fixing the *later* corruption.
  - Stashing register N (used internally by `SAVEX`'s shared body) in
    B across the `gosub SAVEX` call and restoring it before `golong
    ERR110` - also did not help.
  - The true root cause inside `ERR110`'s own file-name-aware display
    code (the same code area Phase 2's unresolved RESZFL/`ERR110` hang
    pointed at) was not pinned down at the individual-instruction
    level - this is the same "root-cause not found, but a clean,
    verified workaround exists" outcome as that earlier bug.

**Consequence, now implemented in `src/frames.s`**: `LSTO` ends with a
bare `rtn`, not `golong ERR110` - it does NOT refresh the display (X is
left showing whatever it already displayed), the same "no visible
feedback" tradeoff already accepted for LCLS/LCLX's silent-refusal
path. `LRCL` is unaffected and uses `golong ERR110` as normal.

**A real Calypsi FAT-table consequence, already known but reconfirmed
here**: `Lsto`/`Lrcl` had to be inserted *before* `Padding` in the
`.fat` list, per the Phase 2 "last `.fat` entry never dispatches"
finding - `Padding` must always stay last.

**Closed out Phase 1's own deferred naming-collision check** before
tagging: Phase 1's tag message explicitly left "LCLS/LCLX/LSTO/LRCL
naming... deferred to Phase 3, checked against the `.modexport`
name-collision data... before anything is finalized." Grepped all 55
real module `.modexport` files (`toolchain/calypsi-nut-5.18/module-
export/`) for exact-name matches (case-insensitive) against `LCLS`,
`LCLX`, `LSTO`, `LRCL`, `MFSTK`, and `MFPAD` - **zero collisions**.
Names are final as used.

**Real, honest scope limits of what Phase 3 has NOT solved**:
- `LSTO`/`LRCL` do not embed a slot number themselves (unlike real
  `STO 00`-`STO 15`) - the calling FOCAL program computes the absolute
  register (`frame_base = current_size - 3`, slot 0-3 = `frame_base`
  through `frame_base + 3`) and does its own `SEEKPTA` before every
  access that isn't the immediate next register in file order. This is
  a real, inherent HP-41 XM constraint (`SEEKPTA` is architecturally a
  FOCAL-program-level operation - Finding 1 above rules out any
  MCODE-internal alternative), not a gap specific to this design - but
  it does mean the ergonomics fall short of the original vision of a
  clean, hidden-plumbing `LSTO`/`LRCL` interface. Worth revisiting
  later (e.g. slot-literal FAT entries `LSTO0`-`LSTO3`/`LRCL0`-`LRCL3`,
  matching the real Geir Isene-cited "PPC ROM packs many functions into
  short names" precedent) but explicitly deferred, not solved now.
- The exact root cause of Finding 5's `SAVEX`+`ERR110` corruption
  inside the OS's own code was not found - only a verified workaround.
- Compatibility (native FOCAL programs unaffected by MultiFOCAL's
  presence) is still untested, unchanged from the Phase 2 tag's own
  scope note.
**Phase 3 is now tagged complete** (`phase-3`).

## Phase 4: LRST, manual recovery from an orphaned frame (2026-09-05)

Started Phase 4 at the user's direction, choosing between it and
compatibility testing (both open since Phase 2/3) - Phase 4 won because
it closes a known hole in what's already built, where compatibility
testing only verifies nothing broke.

**A real prerequisite gap surfaced first, not previously written down
anywhere**: `LCLS`/`LCLX` are pure X-in/X-out functions - every test so
far (`frames_test.c`, `lsto_lrcl_test.c`) feeds the returned size
straight back into the next call, but a *real* FOCAL subroutine uses X
for its own arithmetic between push and pop. Nothing in Phases 1-3
actually specified where the persistent "current logical stack size"
lives across that gap. Resolved here as **documentation, not new
MCODE**: the calling FOCAL program is expected to hold it in a
dedicated variable (recommended name `MFSZ`), STOing into it right
after `LCLS` and RCLing right before `LCLX`/`LSTO`/`LRCL`'s own
register-address math. This is exactly the value that goes stale on
abrupt exit - Phase 4's real question is downstream of this one.

**The abrupt-exit question itself**: if a subroutine calls `LCLS` then
never reaches its matching `LCLX` - a FOCAL error abort (any op error
halts the running program and returns to the keyboard), a `GTO` out of
the subroutine, or a manual stop - `MFSZ` is left stuck at the elevated
size. Nothing is corrupted (`LCLS`/`LCLX` never touch XM at all - pure
counter arithmetic, per the Phase 2 redesign above), but the frame's
registers are permanently "lost": every future `LCLS` silently builds
on top of the stale size instead of reusing the orphaned space, and
this compounds until the depth ceiling refuses pushes for no logical
reason - silently, per the existing LCLS/LCLX refusal convention.

**Design choice, made with the user's sign-off after the tradeoffs were
laid out**: no automatic detection. This project's own history -
the RESZFL/`ERR110` hang and the `SAVEX`+`ERR110` corruption bug, both
documented above - is a repeated, hard-won lesson that reaching into OS
internals (here, that would mean patching the error-handling/abort
path) is exactly where this project has hit its worst, hardest-to-
diagnose bugs on real hardware semantics. MultiFOCAL also has no way to
tell "legitimately empty stack" apart from "orphaned stack" on its own.
Instead: a new manual recovery function, **`LRST`**, that the FOCAL
programmer calls at a known-safe recovery point (top of the main
program, an explicit reset/menu routine) - the same discipline Geir
Isene's own coding standard already calls for ("every routine must
return to the header... as its last step," see the community-convention
research above) applied one level up to MultiFOCAL's own frame stack.

**Implementation** (`src/frames.s`): `LRST` takes no meaningful input -
it unconditionally writes X = 2 (the empty-stack minimum) regardless of
X's prior value, via the same already-verified `StoreFloatIntoX` +
`golong ERR110` path `LCLS`/`LCLX` use. Deliberately touches no XM
state at all - `MFSTK`'s physical contents are never reclaimed or
reinitialized, only the logical counter a real program threads through
`MFSZ` resets to empty. Because it never calls `SAVEX`, it cannot hit
Phase 3's `SAVEX`+`ERR110` corruption bug - `golong ERR110` here is
exactly as safe as it already is in `LCLS`/`LCLX`.

Inserted into the `.fat` list *before* `Padding`, per the Phase 2 "last
`.fat` entry never dispatches" finding (list is now `Header, Lcls,
Lclx, Lsto, Lrcl, Lrst, Padding`).

**Verified** via a new, dedicated test (`test/frames_lrst_test.c`,
checked in): pushes 3 real nested frames (2->6->10->14) to simulate a
legitimately in-progress call chain, then calls `LRST` directly with X
still at the orphaned value 14 and confirms it returns 2 (not something
derived from 14); confirms idempotency (`LRST` from an already-empty
X=2 also returns 2); confirms normal `LCLS`/`LCLX` operation resumes
correctly from the post-reset state; and confirms `LRST` ignores X even
at the opposite extreme (X=34, the depth ceiling, still resets to 2).
All 8 checks pass. Re-ran all three pre-existing suites
(`frames_test.c`, `frames_bounds_test.c`, `lsto_lrcl_test.c`) against
the rebuilt module (now with `LRST` inserted before `Padding`) - all
still pass, no regressions.

**Real, honest scope limits of Phase 4**:
- `LRST` is purely reactive - nothing detects that a frame *was*
  orphaned or warns the user; calling it at the wrong time (e.g. while
  frames are still legitimately in use) silently discards them exactly
  like an orphan would, since MultiFOCAL cannot distinguish the two
  cases. This is a deliberate, sign-off tradeoff (see above), not an
  oversight.
- The `MFSZ` convention documented here is a recommendation for how a
  real FOCAL program should use `LCLS`/`LCLX`/`LSTO`/`LRCL` together -
  it is not itself checked, enforced, or read by any MultiFOCAL MCODE;
  a program is free to use a different variable name or storage
  scheme, as long as it's consistent.
- No real, hand-written FOCAL program demonstrating this whole
  convention end-to-end (as opposed to the C test harness's synthetic
  keystroke sequences) has been written yet.
- The automatic-detection alternative (an OS-level error/abort hook)
  was considered and explicitly declined for this phase, not proven
  infeasible - see the design-choice paragraph above.
- Compatibility (native FOCAL programs unaffected by MultiFOCAL's
  presence) remains untested project-wide, unchanged from every prior
  phase's own scope note.

## Compatibility testing (2026-09-05)

Started at the user's direction, choosing this over further Phase 5+
feature work since it's the one hard constraint (`CLAUDE.md`'s own
"native FOCAL programs must behave identically whether or not
MultiFOCAL is present") that's been flagged as untested since the
Phase 2 tag and never actually acted on.

**A real tooling constraint found first**: neither soynut's
`hp41_key_bridge.c` (the shared wire-protocol parser this project's
whole test harness is built on) nor its `named_keys[]`/`tabcode[]`
tables, nor `sim/sim_keyboard.c`'s SDL key map, expose a way to press
STO/RCL/GTO/LBL as their own distinct physical keys - only ASCII
letters/digits/operators (via `tabcode[]`) and a small named-key set
(`ON`, `USER`, `PRGM`, `ALPHA`, `SHIFT`, `SST`, `BST`, `RS`, `XEQ`,
`CLX`, `XY`, `RDN`). Confirmed by reading soynut's own vendored
`emu41gcc/emu41.c` (`traite_touche()`, the DOS reference emulator this
table was transcribed from unchanged) - the original DOS emulator
itself never mapped STO/RCL/GTO to any PC key either. Since `~/soynut`
is read-only for this project (see this file's own "Relationship to
`~/soynut`" section), a real hand-written stored FOCAL program
exercising STO/RCL/GTO by name was out of scope for this pass - not
attempted, not something this session invented a workaround for.

**What was tested instead, and why it's still a real, meaningful
check**: the actual compatibility-sensitive risk a MCODE module
introduces is (1) whether its mere presence in a FAT page changes
catalog dispatch or any other native operation's outcome, and (2)
whether its own XM usage corrupts a native program's independent XM
file via the shared global "current file" state Phase 2 groundwork
flagged from day one. Both are testable with operations the harness
already reliably drives (ALPHA-name entry, digit entry, real RPN
arithmetic via `tabcode[]`'s `+`/`-`/`*`//` operators, and real catalog
`XEQ` of native CX functions like `CRFLD`/`SAVEX`/`GETX`/`RESZFL`),
without needing STO/RCL/GTO specifically.

**Test 1 - presence-only invariance** (`test/compat_presence_test.c`,
`test/run_compat_presence.sh`): runs an identical sequence of native
operations (create+use its own `NATIVE` XM file via `CRFLD`/`SAVEX`/
`GETX`/`RESZFL`, plus real RPN arithmetic `5 ENTER 3 + 2 - 4 * 3 /`)
twice - once with MultiFOCAL's module loaded at page 8 (every other
test's own boot pattern), once with page 8 left untouched entirely
(confirmed by inspection: `nut_boot_cx()` never touches page 8 itself -
every existing test sets `tabpage[8]`/`typmod[8]` explicitly after
calling it, so "module absent" is simply not doing that). The two
runs' full stdout (every intermediate display plus a checksum of the
CX's 128 built-in XM registers) are diffed. **Result: byte-for-byte
identical.** MultiFOCAL's module, when not invoked, has zero observable
effect on native operation.

**A real anomaly found while building Test 1, investigated to a
confirmed root cause immediately afterward (not left open)**: after
`SEEKPTA` to register 1 of a freshly-populated file (`SAVEX`'d with 11,
22, 33 via real catalog dispatch, immediately after `CRFLD`, in that
order), the first real catalog `GETX` call returned 22, not 11 - as if
a value had gone missing. **Root cause, confirmed via a dedicated probe
(three separate checks, not checked in - reproducible from this
description): a fresh `CRFLD` does NOT leave the file's pointer
positioned to write register 1 directly.** The probe confirmed, with
no ambiguity: (1) `SEEKPTA(n)` immediately followed by a single `SAVEX`
or `GETX`, done individually and separately for `n` = 1 through 5 with
a fresh reseek before every single op, gives an exact 1:1 correspondence
- register `n` is written/read, every time, no exceptions; (2) a
*chain* of `SAVEX`es or `GETX`es all issued after a single, explicit,
upfront `SEEKPTA(1)` - no reseek in between - is *also* exact and
correctly sequential (11,22,33,44,55 in, same values out in order); (3)
but a `SAVEX` chain issued **right after a bare `CRFLD`, with no
explicit `SEEKPTA` in between**, loses its first value - the first
write lands outside the file's normal register range entirely (not
merely at a shifted register - a directly-confirmed absolute
correspondence test found no value 11 anywhere in registers 1-5
afterward). **This is a real, general HP-41CX behavior, unrelated to
MultiFOCAL**: `CRFLD`'s own post-creation pointer state is not
equivalent to `SEEKPTA(1)` - always issue an explicit `SEEKPTA` right
after `CRFLD`, never assume the pointer is ready. Both compatibility
test files were fixed to do this (`compat_presence_test.c`,
`compat_xm_coexist_test.c`) and now read back exactly the values
written, no discrepancy. **This does not, and never did, affect
Phase 3's own design**: the documented `LSTO`/`LRCL` calling convention
already mandates an explicit `SEEKPTA` before every single access (see
this file's own Phase 3 section) - `lsto_lrcl_test.c` never relied on
`CRFLD`'s implicit pointer state, so it was never exposed to this
quirk. Only this session's two new, quickly-written compat tests had
the gap, and both are now fixed and re-verified passing.

**Test 2 - XM coexistence** (`test/compat_xm_coexist_test.c`): targets
the specific, previously-flagged shared-global-state risk directly -
does a native program's own XM file survive a real subroutine call
that uses `LSTO`/`LRCL` (which, unlike the pure-arithmetic `LCLS`/
`LCLX`, do touch XM via `SAVEX`/`REALGETX` against whatever file is
currently "current") in between? Also fixed to `SEEKPTA` right after
its own `CRFLD` (per the root-caused finding above), and kept
deliberately self-referential anyway rather than hardcoding an
assertion on specific numbers - good practice regardless of whether the
values are now well understood: reads a native `NATIVE` file's contents
via the same `SEEKPTA`+`GETX` sequence twice - once immediately after
writing it (the "baseline"), once again after a full round of
MultiFOCAL activity (`CRFLD MFSTK`, 3x `LCLS`, a seek, 2x `LSTO`, 3x
`LCLX`, which makes `MFSTK` "current" partway through) - and asserts
the two reads match exactly. **Result: PASS** - `NATIVE` reads back
identically (91, 92, 93 both times, now correct after the `CRFLD`/
`SEEKPTA` fix) after `MFSTK` was created, seeked into, written to, and
left "current" for most of the intervening sequence. MultiFOCAL's own
XM operations do not leak into or corrupt an independent native file.

Re-ran all 4 pre-existing suites (`frames_test`, `frames_bounds_test`,
`lsto_lrcl_test`, `frames_lrst_test`) after adding these - all still
pass, no regressions (expected, since nothing in `src/frames.s` itself
changed this pass).

**Real, honest scope limits of this compatibility-testing pass**:
- No real, hand-written stored FOCAL program (entered via `PRGM` mode,
  `LBL`/`GTO`/`STO`/`RCL` keystrokes) was run - blocked by the shared
  test harness's own keyboard-mapping gap (see above), which this
  project does not own and does not modify. What was tested instead
  (native catalog dispatch + real arithmetic) covers the two concrete
  risk vectors a MCODE module actually introduces, but is not the same
  thing as a literal "native FOCAL program."
- Catalog *display* effects (whether `CAT 2` lists MultiFOCAL's
  functions differently than a bare CX) were not tested - out of scope
  for the "native programs behave identically" wording, since listing
  differences don't change program *execution* results.
- This pass tests the CX boot configuration only, same as every phase
  before it - the CV+82180A real-hardware question from this file's
  "Real-hardware testing target" section remains completely separate
  and still unverified.

## Real HP-41 keycodes found: STO, RCL (2026-09-05)

Follow-up to the tooling gap identified in compatibility testing above
(no way to press STO/RCL/GTO/LBL as physical keys). Investigated what
it would actually take to fix it, at the user's direction.

**Key realization making this tractable at all**: `dokey()` in
`emu41gcc/nutcpu.c` pushes `keybuffer[0]` straight into `regK` with no
further translation - the codes `hp41_key_bridge.c`'s `tabcode[]`/
`named_keys[]` tables carry are genuine HP-41 hardware key-matrix
codes, not an emulator-only abstraction. So STO/RCL/GTO/LBL have real,
discoverable numeric codes - they're just missing from both vendored
reference tables this project can read (confirmed: soynut's own
vendored `emu41gcc/emu41.c`, the DOS reference emulator `tabcode[]` was
transcribed from unchanged, never mapped these either - not a gap
specific to `hp41_key_bridge.c`).

**Approach**: a small, MultiFOCAL-owned raw-keycode injection technique
(`test/hp41_raw_keys.h`) that pushes a candidate byte directly into
`keybuffer[]`/`lgkeybuf`, bypassing `hp41_key_bridge.c`'s restricted
tables entirely - no soynut modification. Brute-forced the full
0x00-0xFF byte range against the real ROM, using genuinely observable
effects (a real named prompt spelling itself out on the display, or a
clean register round-trip) rather than guessing from the ASCII table's
structure - an early attempt to reverse-engineer the key-matrix
row/column layout from `tabcode[]`'s letter/digit overlaps produced an
internally inconsistent picture and was abandoned in favor of direct
empirical brute force, consistent with this project's established
practice.

**A real, serious methodology bug found and fixed along the way**:
`nut_boot_cx()` only rewires ROM pages (`tabpage`/`typmod`/`tabbank`)
and a handful of CPU registers (`regPC`, `regST`, `Carry`,
`mode_printer`) - it does **not** clear `espaceRAM`, `keybuffer`,
`lgkeybuf`, `flagKey`, or any other CPU/memory state. Every test in
this project before this session called it exactly once per process,
so this was never noticed. The first brute-force attempt looped many
candidates with a fresh `nut_boot_cx()` call *within one process* and
got wildly unreliable results (66 false-positive "hits" for STO alone)
from cross-iteration contamination - confirmed directly: re-running the
exact same single-candidate sequence in true isolation (a fresh
process) gave a different, reproducible answer than the contaminated
loop did. **Fix, and the pattern every probe here now follows: one
candidate per process**, matching how every other single-boot test in
this suite already worked safely. `test/sto_rcl_test.c`'s own first
version made this exact mistake too (three trials, one `nut_boot_cx()`
call per trial, all in one `main()`) and got 2 of 3 trials wrong before
being caught and fixed the same way - `test/run_sto_rcl.sh` now drives
one process per trial.

**Confirmed, via a decisive, closed-loop test** (STO writes a value,
`CLX` clears X, RCL reads it back - `test/sto_rcl_test.c`, checked in,
run via `test/run_sto_rcl.sh`, 3 independent value/register
combinations, all correct):
- **`STO` = `0x52`** - also directly confirmed via the real prompt
  literally spelling itself out on the display, `"S T O   5 _"`,
  exactly matching real HP-41 keystroke UI.
- **`RCL` = `0x82`** - same direct confirmation, `"R C L   _ _"`.
- **A real HP-41 convention confirmed the same way**: both always
  need a full 2-digit register number (e.g. `"05"`, not `"5"`) - a
  single digit leaves the prompt open without committing (confirmed
  directly: register content stayed unchanged after a single-digit
  entry, despite the display no longer looking like an obvious
  mid-entry state in every case).

**GTO and LBL found in a follow-up session, continuing from the
`0xd0`/`0xd6` lead above.** The first pass's methods (a plain-
calculator-mode name-spelling scan, and an inconclusive program-branch
test) both had real gaps, closed here:

- **The plain-mode name-spelling scan was the wrong context.** GTO
  does not spell its own name when pressed in ordinary calculator mode
  (confirmed: a full rescan of that exact context still finds nothing) -
  but it DOES spell itself out while a program is being **recorded**
  (`PRGM` mode), a context the first pass's scan never tried. Rerunning
  the "key `1`, then candidate" scan with `PRGM` mode on turned up real
  function names all over the place that the plain-mode scan had
  missed entirely (`SQRT`, `LN`, `SIN`, `COS`, `TAN`, `1/X`, `X<>Y`,
  `ENTER^`, and - the actual target - `"0 2   G T O   1 3"` at
  candidate **`0xd6`**.
- **A second real discovery along the way, needed to even interpret
  that result correctly**: typing several digits in a row while
  recording does NOT create one program line per digit - they merge
  into a single numeric-literal line (confirmed directly: keying
  `1`,`2`,`3` in sequence recorded as ONE line, `"01   1 2 3"`, not
  three). Multiple digits only split into separate lines when a
  non-digit keystroke (a real key like `ENTER^`) intervenes. This
  invalidated the first pass's whole line-counting assumption for its
  program-branch test - not a fault in the idea of that test, just a
  wrong model of what it was actually building.
- **GTO's own argument-entry behavior is genuinely different from
  STO/RCL's, which is why the "13" surprised us**: pressing GTO while
  recording commits IMMEDIATELY, with a default target already filled
  in (here, `13`) - it does NOT open an interactive digit-entry window
  the way STO/RCL do (confirmed directly: typing digits or `.NNN`
  right after GTO starts a **new, separate** program line instead of
  modifying GTO's own target; pre-loading X beforehand doesn't change
  the default either - `13` is a fixed recording-time UI default,
  unrelated to X). This is a real, useful finding in its own right,
  not just a workaround: **rather than fight the default, the
  confirming test uses it as-is** and places a matching target there.
- **Decisive, positive confirmation, not just a name coincidence**: a
  program `"1, GTO 13, 5, +, LBL 13"` (LBL = SHIFT+STO, see below),
  run from the top, leaves X=1 - the `5,+` arithmetic between the GTO
  and its target is completely skipped. The negative control already
  existed from the first pass: the exact same `GTO 13` with no matching
  `LBL 13` anywhere in the program produces a real `"NONEXISTENT"`
  error when executed (traced via `SST`, not just inferred) - i.e.
  GTO's behavior is genuinely target-sensitive, not a fixed no-op that
  happens to look like success. Checked in as `test/gto_lbl_test.c`.
- **LBL = SHIFT+STO, not SHIFT+GTO** - a real, corrected assumption.
  The `BST`=SHIFT+SST pattern suggested trying SHIFT+GTO first; that
  produced a plain digit instead (confirming shift doesn't uniformly
  reuse the same code for every key - `BST` is a documented special
  case, not the general rule). The real answer was found the same way
  GTO itself was: rerunning the PRGM-mode name-spelling scan with
  `SHIFT` held before each candidate turned up a real `"L B L  _ _"`
  prompt at **three separate codes** (`0x22`/`0x52`/`0x72` - electrical
  key-matrix aliases of the same logical key) - `0x52` among them is
  the already-confirmed `STO`, a clean three-way agreement.

**Net effect on the original tooling-gap question: fully closed for
STO/RCL/GTO/LBL.** All four are real, confirmed, permanent
capabilities now (`test/hp41_raw_keys.h`, `test/sto_rcl_test.c`,
`test/gto_lbl_test.c`) - enough to build a genuine stored FOCAL program
with storage, recall, and branching. What a literal "real stored FOCAL
program" compatibility test would still need beyond this (arithmetic,
`ENTER^`, and digit entry were already usable via plain `tabcode[]`
characters) is mostly just assembling these primitives into an actual
test - not blocked on any further keycode discovery.

## Compatibility testing, Test 3: a literal stored FOCAL program (2026-09-05)

Closed the gap Test 1 explicitly could not attempt (see its own scope
notes above) now that STO/RCL/GTO/LBL are all confirmed.

**The program** (`test/compat_native_program_test.c`, 9 real lines,
entered via genuine `PRGM`-mode keystrokes - verified independently in
a throwaway probe before being locked in): store 5, recall it, enter 3
(a fresh digit entry automatically lifts the stack, per standard RPN
semantics - `Y=5, X=3`), `GTO 13` past a deliberately wrong `999`
line, land on a matching `LBL 13`, add (`5+3=8`, only reachable this
way if the `999` line was genuinely skipped), store the result. Run
once with MultiFOCAL's module present at page 8, once with page 8
untouched (same pattern as Test 1), diffing full stdout - every
intermediate recording-time display, the post-run display, and two
independent `RCL` checks (`R01=5`, `R02=8`) confirming the registers
actually hold the right values, not just that the display happened to
show a plausible number. **Result: byte-for-byte identical, and
correct** (`test/run_compat_native_program.sh`). This is the most
literal reading yet of the "native FOCAL programs must behave
identically" constraint - an actual stored, recorded, GTO-branching
FOCAL program, not calculator-mode keystrokes standing in for one.

Re-ran all 9 pre-existing suites - all still pass, no regressions
(expected, nothing in `src/frames.s` changed this pass).

**Compatibility testing is now substantively complete** across all
three angles identified: presence-only invariance (Test 1), XM
coexistence (Test 2), and a literal stored program (Test 3). The only
remaining, explicitly out-of-scope items are catalog/`CAT`-listing
display differences (not an execution-behavior question) and the
CV+82180A real-hardware question, which is a separate, still-untouched
undertaking.

## Real HP-41CV+82180A boot config: major progress, one real
architectural gap confirmed (2026-09-05)

The user has the real HP-41CV hardware on hand but currently no way to
load software onto it (no EPROM burner/TULIP4041 yet) - real-hardware
testing is blocked on that, separately from anything code-side. But
the user *sourced a real 82180A.MOD* (placed in `~/soynut/roms/`),
making genuine emulated verification of the actual real-hardware
target possible for the first time - the direct verification Phase 1
flagged as needed back at the very start of this project.

**The ROM is confirmed genuine**: `modtool --summary` on
`~/soynut/roms/82180A.MOD` shows XROM 25 "Extended Functions/Memory
Module" (HP's real part number 82180A) with exactly the expected
function set - `CRFLD`, `SEEKPTA`, `RCLPTA`, `GETX`, `SAVEX`, `GETRX`,
`SAVERX`, and the rest this project has used against the CX's built-in
`CXFUNS` ROM since Phase 2. Single page (`rom_82180a_p0`), MOD1 header
declares `Page=Any` (genuinely port-pluggable, not fixed to one slot -
consistent with the port-pluggable design decision already on record).

**Integration, mirroring existing conventions**: a new top-level
Makefile target (`cv82180a-roms`) converts the MOD file via soynut's
`mod_to_c.py` (MOD1-aware, unlike `rom_to_c.py`) into
`build/e82180a_rom.c`. Reuses soynut's own `nut_boot()`
(`firmware/emu41gcc_compat/nut_rom.c` - its own doc comment already
calls it "the base HP-41CV OS ROM", backed by soynut's own already-
built `roms/rom_images.c`) for the base OS, rather than duplicating it -
a cleaner fit than the `nut_boot_cx()` pattern used for the CX, since
that function already exists and is already used by
`phase0_loop_test.c`. New `test/nut_rom_82180a.h`/`.c` wires the module
ROM in, mirroring `nut_rom_multifocal.c`'s "separate wiring function,
call after the base boot" pattern.

**A real, well-corroborated finding: Port 1 (page 4) is broken for
ANY module, not specific to the 82180A.** First attempt wired the
82180A at page 4 (Port 1, the natural first choice) - cold boot never
reached "MEMORY LOST" or any stable state; instead the display showed
a non-monotonic, never-converging pattern involving a `Σ+` annunciator
and an erratic counter, persisting even after 700,000+ instructions and
unaffected by a real keypress. A disassembled instruction trace
(`desas()`, the same disassembler `xm_trace_test.c` already uses)
showed the base OS polls page 4 within the first handful of cold-boot
instructions (`GSUBNC 4000` at PC≈0x1AD) - the module's own poll-vector
code there executes cleanly and returns immediately every single time
(confirmed: only 3 total accesses into page 4 in a 5000-instruction
trace, always the identical one-instruction response) - the
instability is entirely within the *base OS's own* post-poll logic
(observed directly: `REGN=C` writes to registers 0/1/2 - the T/Z/Y
stack registers - recurring 50 times in 5000 instructions, consistent
with a repeating stack-clear/reset cycle that never completes).
**Decisively ruled out as 82180A-specific**: wiring MultiFOCAL's own,
already-extensively-verified `frames.mod` at page 4 instead reproduces
the *identical* failure. This is independently consistent with
`nut_boot_cx()`'s own comment that "page 4 is not used by this
configuration at all" for the CX mainframe - two separate pieces of
evidence agreeing Port 1/page 4 has real, special-purpose early-
cold-boot significance and is not a safe general-purpose port. **Fix:
use Port 2 (page 5) instead** - confirmed clean, alone and alongside
MultiFOCAL's own module at Port 3 (page 6), a genuine two-module
physical configuration.

**Milestone reached: the real 82180A module provides working XM on a
plain HP-41CV boot** (`test/cv82180a_smoke_test.c`) - a real
`CRFLD`/`SEEKPTA`/`SAVEX`/`SEEKPTA`/`GETX` round trip via keystrokes,
correct on the first clean run once the already-known "always
`SEEKPTA` right after `CRFLD`" convention (from compatibility testing
above) was applied here too.

**Bigger milestone: MultiFOCAL's own `LCLS`/`LCLX` frame push/pop
works correctly on the real target architecture**
(`test/cv82180a_frames_test.c`) - the exact same 6-step milestone
(`2→6→10→14→10→6→2`) `frames_test.c` already verified on the CX,
reproduced identically here. This is the first time any of
MultiFOCAL's own MCODE has been shown working on anything other than a
CX - genuinely answering Phase 1's "HP-41CV's XM compatibility is
genuinely unclear... needs direct verification" question, at least for
this piece.

**A real, previously-flagged-but-never-actually-tested architectural
gap, now confirmed**: `LSTO`/`LRCL` do **NOT** work on this
configuration (`test/cv82180a_lsto_lrcl_test.c`, currently failing,
not yet fixed or committed as a passing test) - `LRCL` reads back
stale/wrong values (the seek-target digit, never the actually-stored
value) instead of erroring outright. **Root cause, confirmed by
reading `src/frames.s` directly**: `Lsto`/`Lrcl` call `gosub SAVEX` /
`gosub REALGETX`, and both symbols come from `#include
"mainframe_cx.h"` - **fixed, hardcoded CX-mainframe addresses** (page
3). This is *exactly* the risk Phase 2 groundwork explicitly flagged
before any MCODE was even written: "On a base HP-41C + the 82180A
module, the identical functions live in a plug-in module at a port-
dependent address - a hardcoded GOSUB to `mainframe_cx.h`'s address...
would be silently wrong on that configuration." That warning was
heeded for `CRFLD` (dispatched via real keystrokes, never `gosub`'d) -
but `LSTO`/`LRCL`'s own internal `SAVEX`/`REALGETX` calls, added later
in Phase 3, silently reintroduced the exact same class of risk, and it
went undetected because Phase 3's own verification never ran against
anything but a CX. Confirmed directly: address `0x3805`/`0x380B` sit
in page 3, which is genuinely unpopulated in this CV+82180A config
(never wired in any of these tests) - `gosub`ing there jumps into
empty ROM (reads as zero words), executing something inert rather than
the real routine, which is exactly the "silently wrong," not crashing,
symptom observed. **The real fix** (not yet implemented): dispatch to
`SAVEX`/`GETX` by their XROM number (`25,41` and `25,23` respectively,
per `modtool --summary`'s own output above) rather than a hardcoded
absolute address - the portable, "ordinary cross-module call"
mechanism the original kickoff brief and Phase 2 groundwork both
already anticipated as the correct approach, still not yet actually
implemented anywhere in this codebase. `LCLS`/`LCLX` were never
exposed to this risk at all (confirmed, again, in this session): they
never call `SAVEX`/`GETX`/any mainframe-fixed address - pure X-register
arithmetic - which is exactly why they passed cleanly on the first try
here while `LSTO`/`LRCL` did not.

**Honest scope of what's verified vs. not, as of this checkpoint**:
- Verified, real, working: the sourced ROM's authenticity; the CV+
  82180A boot config (once page 4 is avoided); `LCLS`/`LCLX` on the
  real target architecture.
- Confirmed broken, root-caused, not yet fixed: `LSTO`/`LRCL`'s
  hardcoded-CX-address `gosub` calls. `LRST` almost certainly has the
  same exposure as `LCLS`/`LCLX` (pure arithmetic, no XM calls) but
  hasn't been independently re-verified against this config yet.
- Not yet attempted: fixing `LSTO`/`LRCL` to use portable XROM
  dispatch, then re-verifying both this config AND the CX config still
  pass (a real regression risk if the fix is wrong, since the CX
  config depends on `LSTO`/`LRCL` too).
- Real hardware itself remains untested - the user has no deployment
  path (EPROM burner/TULIP4041) yet; everything above is emulated
  verification only, using the user's own legitimately-sourced ROM.

## LSTO/LRCL retired; local-variable access now uses SAVEX/GETX
directly (2026-09-05)

Closes the gap from the section above, at the user's direction after
weighing two alternatives (a self-contained FAT-table scanner inside
`LSTO`/`LRCL` themselves, vs. continuing to reverse-engineer
`mainframe.h`'s `XROM`/`XROMNF` utility at `0x2FAF`/`0x2F6C` -
disassembled partway but genuinely inconclusive: Calypsi's own RPN-
compiler docs confirm "XROM rn,fn" is fundamentally a FOCAL *program-
step* encoding the interpreter consumes, not a simple MCODE-callable
subroutine with an obvious register convention, so fully understanding
it safely looked like its own multi-session investigation).

**The decision, and why it costs nothing functionally**: `LSTO`/`LRCL`
never embedded a slot number or computed the target register
themselves - by design, that was always the calling FOCAL program's
own job (`SEEKPTA` first, always). They were pure `gosub` pass-throughs
to `SAVEX`/`GETX` with no logic of their own beyond a corruption-bug
workaround (`LSTO`'s bare `rtn` instead of `golong ERR110`) that a real
catalog dispatch doesn't even need - already confirmed back in Phase 3:
real `XEQ SAVEX` has no corruption issue, only `gosub SAVEX; golong
ERR110` did. So retiring them and having the calling program use the
real `SAVEX`/`GETX` directly is not a workaround or a downgrade - it's
simpler (one fewer name to learn), fixes a real limitation (`LSTO`'s
suppressed display refresh goes away, since real `XEQ SAVEX` completes
normally with visible feedback), and is portable by construction
(ordinary catalog dispatch resolves to whichever page actually holds
the XM-providing module, on any hardware). The one real cost: Phase 3's
own deferred "slot-literal ergonomics" idea (`LSTO0`-`LSTO3` embedding
the register offset) depended on `LSTO`/`LRCL` existing as MultiFOCAL's
own functions to build on - if ever revisited, it needs a different
design now.

**Implementation** (`src/frames.s`): removed the `Lsto`/`Lrcl` FAT
entries and bodies entirely (list is now `Header, Lcls, Lclx, Lrst,
Padding`), removed the now-unused `REALGETX` equlab, and removed the
`#include "mainframe_cx.h"` line - **this module now makes zero
hardcoded-address `gosub` calls into any CX-mainframe-specific routine
at all**, the real portability property the whole exercise was after.
The Phase 3 header comment block was rewritten in place (not deleted)
to explain the retirement and preserve the parts that are still true
and load-bearing for direct `SAVEX`/`GETX` use: the size-35 `MFSTK`
requirement (the last-register `SEEKPTA` bug is unrelated to `LSTO`/
`LRCL` and still applies), and the "caller must `SEEKPTA` first, real
FOCAL step, never `gosub`'d" calling convention - now illustrated with
`XEQ SAVEX`/`XEQ GETX` directly instead of `XEQ LSTO`/`XEQ LRCL`.

**Verification, both configurations, all green**:
- CX: `test/local_slot_access_test.c` (renamed from `lsto_lrcl_test.c`,
  same 8-frame depth-ceiling coverage, `xeq("LSTO")`/`xeq("LRCL")`
  replaced with `xeq("SAVEX")`/`xeq("GETX")`) - PASS, byte-identical
  results to the original.
- CV+82180A: `test/cv82180a_local_slot_test.c` (renamed from
  `cv82180a_lsto_lrcl_test.c`, which had been left uncommitted as a
  known-failing test documenting the bug) - **now PASSES**, confirming
  the retirement genuinely fixes the real-target compatibility problem,
  not just removes the symptom.
- `test/compat_xm_coexist_test.c` (compatibility Test 2) updated the
  same way (its "MultiFOCAL activity" step used `LSTO`, now `SAVEX`) -
  still PASSES.
- All other suites (`frames_test`, `frames_bounds_test`,
  `frames_lrst_test`, `gto_lbl_test`, `cv82180a_smoke_test`,
  `cv82180a_frames_test`, `compat_presence_test`+script,
  `sto_rcl_test`+script, `compat_native_program_test`+script) re-run
  and still pass - no regressions from removing two FAT entries.

**Current, correct status** (supersedes the "confirmed broken, not yet
fixed" checkpoint above): `LSTO`/`LRCL` no longer exist. Local-variable
storage/recall is `XEQ SAVEX`/`XEQ GETX` directly, preceded by the same
`SEEKPTA` step as before. Both the CX and the real CV+82180A
architecture are verified working with this convention. `LRST` was
also independently verified against the CV+82180A config
(`test/cv82180a_lrst_test.c`) - PASS, confirming the inference (it
shares `LCLS`/`LCLX`'s exact code pattern, no `SAVEX`/`GETX`/any
mainframe-fixed address at all) with an actual test result, not just
code inspection. Every one of `LCLS`/`LCLX`/`LRST`/local-slot access
now has dedicated CV+82180A test coverage.

## A real, hand-authored demonstration FOCAL program - and a real bug
it found in already-tagged MCODE (2026-09-06)

Closed the "real hand-written demonstration program" gap this file has
flagged since the compatibility-testing pass: `test/demo_local_scoping_test.c`
is a genuine, hand-authored, stored FOCAL program (two real global
labels, `LBL "MFDEMO"` and `LBL "INNER"`, entered via real `PRGM`-mode
keystrokes, `LBL "MFDEMO"` calling `XEQ "INNER"` as a real subroutine
call) that demonstrates MultiFOCAL's actual value proposition end to
end: `MFDEMO` pushes its own frame and stores 111 into it, calls
`INNER` (which independently pushes its OWN frame, stores 222, and
pops it again), then `MFDEMO` reads its own local back and confirms it
is still 111 - undisturbed by `INNER`'s entirely separate frame
activity. This is the first MultiFOCAL demo that is a real, runnable
FOCAL program a user could type in and `XEQ`, not a C-harness keystroke
sequence standing in for one.

**New keycode found along the way: `RTN` = SHIFT + `0x83`.** A first
guess (SHIFT + the already-known R/S code, by analogy with LBL=SHIFT+
STO) turned out to be `VIEW`, not `RTN` - confirmed wrong, then found
properly via the same brute-force method used for GTO/LBL: a full
`0x00`-`0xFF` PRGM-mode name-spelling scan, **one process per
candidate** (both plain and SHIFT'd - contamination from candidates
that open multi-digit prompts, like `STO IND __`, ruled out doing this
scan within a single process/session). `0x83` unshifted is itself just
an RCL-row alias (`"RCL IND __"`), same pattern as the already-known
STO-row aliases. Recorded permanently in `test/hp41_raw_keys.h`.

**Building this program surfaced a real, previously-undiscovered
correctness bug in the already-tagged `phase-2` MCODE, found and
fixed this session**: the first version of the demo program ran to
completion with all bookkeeping correct (`MFSZ` unwound to 2 exactly
as expected), but the actual XM reads/writes (`SAVEX`(111) at
`MFDEMO`'s own frame register, `SAVEX`(222) at `INNER`'s) silently
read back as 0 - not an error, just wrong, the exact same "ran fine,
values wrong" signature this project has hit several times before.

**Root-caused via careful bisection, the same discipline used
throughout this project**: isolated first to "a real nested subroutine
call between two `SEEKPTA`+`SAVEX` sequences at different registers"
(confirmed fine on its own, via a throwaway two-subroutine probe with
no `LCLS`/`LCLX` involved at all) - then to "adding even one `LCLS`
call anywhere in the running program" (confirmed broken, regardless of
whether the `LCLS` call came before or interleaved with the XM
operations). Progressively stripping `Lcls`'s body down to nothing but
`golong ERR110` (no `gsbp`, no arithmetic, no `setdec`/`sethex` at
all) still reproduced the corruption; replacing that bare `golong
ERR110` with a plain `rtn` made it disappear completely. **Confirmed,
general finding: `golong ERR110` is unsafe as a custom FAT function's
"successful completion" path when the calling context is a RUNNING
FOCAL PROGRAM (as opposed to live/interactive keystrokes)** - it
corrupts something that breaks the OS's own catalog-dispatched XM
calls (`SEEKPTA`/`SAVEX`/`GETX`/`CRFLD`) for the rest of that program's
execution, even though it correctly resumes the calling program's own
next line and even leaves X holding the right value. This went
undetected through every prior phase and the entire compatibility-
testing pass because none of them ever called a MultiFOCAL function as
a step *inside an executing stored program* - every prior test drove
`LCLS`/`LCLX`/`LRST` via live keystrokes only, where the OS's own idle
loop sits between every key and evidently resets whatever `golong
ERR110` leaves broken.

This is a different (though related-in-spirit) corruption from Phase
3's own Finding 5 (`gosub SAVEX` then `golong ERR110` corrupting the
file directory lookup, live keystrokes included) - that one was
specific to interrupting a *native XM routine's* own completion
sequence via `gosub`; this one reproduces with a completely inert
`Lcls` body that never touches XM or calls any native routine at all,
and only manifests in program-context execution, not live. Two
distinct bugs, same underlying lesson, same fix shape.

**Fix, in `src/frames.s`**: `LCLS`, `LCLX`, and `LRST` all now end in
`setdec` (restoring normal decimal mode for the caller as a matter of
hygiene - bisection showed this alone did NOT fix the corruption,
`golong ERR110` itself was the actual cause) followed by a bare `rtn`,
never `golong ERR110` - exactly the fix already established for the
retired `LSTO`. Cost: no more automatic display refresh immediately
after `XEQ LCLS`/`LCLX`/`LRST` (the returned X value itself is
unaffected either way) - a real but purely cosmetic regression,
already an accepted tradeoff elsewhere in this codebase.

**Verified with zero regressions across the entire existing test
suite**, relinked and rerun against the fixed module: `frames_test`,
`frames_bounds_test`, `local_slot_access_test`, `frames_lrst_test`,
`gto_lbl_test`, `compat_presence_test` (+script), `compat_xm_coexist_test`,
`compat_native_program_test` (+script), and all three CV+82180A suites
(`cv82180a_frames_test`, `cv82180a_local_slot_test`,
`cv82180a_lrst_test`) - all still PASS. (Surprisingly, the live display
still shows the correct refreshed value in every one of these live-
keystroke-driven tests even without the explicit `golong ERR110`
refresh - the idle loop's own periodic repaint appears to catch up
within the pump budget these tests already use; Phase 2's original
"bare rtn left the display blank" finding was real at the time but
evidently didn't generalize the way its own writeup assumed.) Then
`demo_local_scoping_test.c` itself: full PASS, all three independent
verification points (`RCL 02`=111, `RCL 01`=2, a fresh `GETX` at
register 3=111) correct.

**Real, honest scope note**: this bug was only found because this
session finally exercised "a MultiFOCAL function called from within a
running FOCAL program" for the first time - every phase before this
one tested exclusively via live keystrokes. Given how many real bugs
this exact gap just surfaced, any *future* MultiFOCAL function should
be verified the same way (as a program step, not just live) before
being considered done, not just live-keystroke-tested as every
function up to now was.

## Interactive simulator: `sim/`, real CV+82180A config (2026-09-06)

At the user's direction: a real, hands-on counterpart to `test/`'s
automated keystroke injection. soynut's own `sim/` is a host-native
SDL2 simulator (real Nut CPU core, real ROM images, a rendered LCD
window, actual keyboard input) - but its own entry point
(`sim/sim_main.c`) only ever boots the plain base OS, with no
MultiFOCAL module wired into any page.

**Built `sim/mf_sim_main.c`**: a copy of soynut's `sim_main.c` (never
modified in place, per this project's standing "nothing under
`~/soynut` is built or modified" convention), with exactly one
addition - right after the same `nut_boot()` every soynut sim run does,
it wires in the real, user-sourced HP 82180A module (page 5, Port 2)
and MultiFOCAL's own `frames.mod` (page 6, Port 3), the identical
CV+82180A configuration `test/cv82180a_*_test.c` already verified
correct. Confirmed safe: `nut_boot()` only ever touches pages 0-2, so
this wiring survives a CLRMEM-triggered reboot with no extra handling.
Every other line of logic in the file is soynut's own, unchanged.

**New `sim/Makefile`**, modeled directly on soynut's own `sim/Makefile`
and this repo's own `test/Makefile`: reuses soynut's `sim_*.c`/
`firmware/*.c`/`emu41gcc/*.c`/`roms/*.c` sources directly by path
(never copied), builds only `mf_sim_main.c` and this repo's own
`test/nut_rom_82180a.c` wiring helper natively, links in
`build/e82180a_rom.c` and `build/frames_rom.c`.

**Verified working, two ways**:
1. A 2-second run: boots cleanly, reaches `MEMORY LOST`, idles, and
   auto-powers-off exactly like every other correct boot in this
   project - no crash, no hang (the page-4 instability this project
   already ruled out for any module stays correctly avoided).
2. A real functional smoke test over the virtual serial port (the same
   PTY soynut's own `tools/hp41_keyboard_gui.py` would connect to):
   drove `CRFLD`/`SEEKPTA`/`LCLS` via real wire-protocol bytes and
   confirmed the display kept advancing through many distinct,
   changing states (not frozen at one instruction) - the identical
   dispatch path `cv82180a_frames_test.c` already proves correct,
   here exercised through the interactive plumbing instead of the
   in-process test harness.

**Real, honest limitation, inherited from soynut's own
`sim_keyboard.c`**: `STO`/`RCL`/`GTO`/`LBL`/`RTN` have no key mapping
in the SDL window - the same tooling gap this project's own
compatibility-testing session found and worked around via raw keycode
injection (`test/hp41_raw_keys.h`), which only ever mattered for a C
test harness feeding raw bytes directly, not for a real keyboard/SDL
window. Consequence: this simulator is fully usable for live,
interactive experimentation with `LCLS`/`LCLX`/`LRST`/`SEEKPTA`/
`SAVEX`/`GETX` (everything invoked via `XEQ ALPHA <name> ALPHA`, which
*is* mapped), but not yet for keying in a full multi-line stored FOCAL
program (like `demo_local_scoping_test.c`'s own `MFDEMO`/`INNER`)
through the window itself - extending `sim_keyboard.c`'s own mapping
(a MultiFOCAL-local addition, same convention as everything else here)
would close this, not yet done.

See `sim/README.md` for the actual usage instructions (one-time
`CRFLD`/`SEEKPTA` setup, then how to invoke each function from the
keyboard).

## MFINIT: automating the one-time setup (2026-09-06)

At the user's direction, after walking through what the four-keystroke-
group `CRFLD`/`SEEKPTA` setup actually does: is there a way to automate
it? The honest answer had a hard wall in it, and this closes the part
that isn't walled off.

**What's permanently blocked, and why**: MultiFOCAL's own MCODE can
never call `CRFLD`/`SEEKPTA` itself, at any point - Phase 3 already
proved both abandon their caller (jump to idle instead of returning)
when called via `gosub`, and an earlier attempt at automatic file
management *inside* `LCLS` is exactly what caused the Phase 2 RESZFL/
`ERR110` hang. So "bake setup into `LCLS` so it happens automatically
on first use" is a door already closed for real, previously-learned
reasons - not reconsidered here.

**What isn't blocked**: an ordinary stored FOCAL subroutine, since
real FOCAL-level `XEQ` dispatch (as opposed to an MCODE `gosub`) never
had the abandonment problem to begin with - that restriction is
specific to calling these functions from inside MultiFOCAL's own
machine code. `test/mfinit_test.c` (CX) and
`test/cv82180a_mfinit_test.c` (the real target architecture) both
record and verify:

```
LBL "MFINIT"
  "MFSTK" 35 XEQ "CRFLD"
  "MFSTK" 1  XEQ "SEEKPTA"
RTN
```

Both pass, first try, on both hardware configurations - unsurprising
in hindsight (MFINIT is pure FOCAL, no MCODE of its own, so there's no
reason CX vs CV+82180A would matter), but confirmed with an actual test
result rather than assumed, same discipline as everything else here.
Verification goes beyond "it ran without error": both tests do a real
`LCLS` -> `SAVEX` -> `GETX` -> `LCLX` round trip immediately after
`XEQ "MFINIT"` and confirm every value is correct - proof the file
`MFINIT` creates is immediately, fully usable, not just present.

**Net effect**: the one-time setup collapses from four separate
keystroke groups (~15 keystrokes) to a single `XEQ ALPHA "MFINIT"
ALPHA`, for any real FOCAL programmer typing this into an actual
calculator or a stored program.

**Real, honest scope limit, not yet closed**: making `MFINIT` safe to
call on *every* run (silently skipping `CRFLD` if `MFSTK` already
exists, so a user never has to remember whether they've already done
this) would need real research into HP-41 error-trapping - can a FOCAL
program catch and inspect a `DUP FL` error and continue past it
cleanly? Not investigated. Also unrelated but worth noting: `sim/`'s
own SDL window still can't key `LBL`/`RTN` at all (the same
`sim_keyboard.c` mapping gap noted above), so a user driving the
interactive simulator by hand still can't record `MFINIT` themselves
through the window - it would need to be pre-loaded via the same raw
PTY byte-injection technique the test harness uses, or via extending
`sim_keyboard.c`'s own mapping (still not done).
