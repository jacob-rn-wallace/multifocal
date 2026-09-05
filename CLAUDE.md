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
- Not yet tagged - this is a milestone within Phase 3, not a
  Phase-3-complete claim.
