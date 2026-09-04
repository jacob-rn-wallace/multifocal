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
  either way yet.
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
