;;; MultiFOCAL Phase 2 proof-of-concept: LCLS/LCLX frame enter/exit.
;;;
;;; SCOPE CUT: fixed width of 4 registers per frame (not read from X).
;;;
;;; DESIGN (rewritten after a decisive finding - see CLAUDE.md's "Phase
;;; 2" section for the full story): LCLS/LCLX are pure functions over
;;; the FOCAL X-register. The CALLER passes the current stack size in X;
;;; each function returns the new size in X. There is no header register
;;; and no seeking, because of a real, confirmed architectural fact:
;;;
;;; **Any real X-Function that touches the ALPHA register (SEEKPTA,
;;; RCLPTA, CRFLD - all confirmed empirically) abandons its caller and
;;; jumps straight to the OS idle loop when called directly via "gosub",
;;; instead of returning via "rtn" like an ordinary subroutine. Purely
;;; numeric X-register functions (GETX, SAVEX, RESZFL - also confirmed
;;; empirically) return normally.** This was proven with a battery of
;;; minimal, isolated test modules, each doing "gosub <function>" then a
;;; message-display side effect that only appears if control actually
;;; returns - see test/probe*_test.c (not checked in - throwaway,
;;; reproducible from this file's description). This means MCODE can
;;; safely call GETX/SAVEX directly, but CANNOT safely call SEEKPTA/
;;; RCLPTA/CRFLD this way at all - only via a real keystroke/XEQ
;;; dispatch, which is why file creation happens once via the test
;;; harness's own keystrokes, never from inside LCLS/LCLX.
;;;
;;; **A second, later finding retracts "safely call RESZFL directly"
;;; for repeated use, even though a single such call does return
;;; normally**: a bare `gosub RESZFL; golong ERR110` (no other logic at
;;; all) was confirmed, via a minimal isolated reproduction, to hang
;;; the *second* time it happens in a session (regardless of which FAT
;;; function issues it) - the infinite loop is inside `ERR110`'s own
;;; code (a bare `rtn` instead avoids it), not RESZFL's, which always
;;; completes and returns correctly. See CLAUDE.md's "second-call hang
;;; precisely isolated" section for the full evidence. **Consequence:
;;; LCLS/LCLX never call RESZFL at all.** MFSTK is instead allocated
;;; ONCE, at its full maximum size (recursion-depth-ceiling x frame-
;;; width + header - see the test harness's setup keystrokes), via the
;;; same one-time real keystroke/XEQ `CRFLD` file creation already used
;;; - and LCLS/LCLX are pure counter arithmetic over X from that point
;;; on, never touching the XM file itself.
;;;
;;; "Current file" being persistent OS state that survives across
;;; separate XEQ invocations (not just within one call) is still true
;;; and still why file creation only needs to happen once - it's just
;;; no longer relevant to LCLS/LCLX's own bodies, only to whatever
;;; later Phase 3 code (LSTO/LRCL) actually reads/writes registers in
;;; that pre-allocated file.
;;;
;;; Register format: a 14-nibble register is nibble13=sign, nibbles3-12
;;; ="M" field = the 10 mantissa digits (ones digit at nibble 12),
;;; nibbles0-2="X" field = the exponent (NOT the calculator's X-register
;;; value, despite the name - a real name collision found the hard way).
;;; Field M is used for all value arithmetic; field X (nibbles 0-2)
;;; remains correct for DADD=C's address argument specifically.
;;;
;;; Calling convention across "gsbp" (page-relocatable local call): gsbp
;;; clobbers C while computing its jump target, so values are relayed
;;; through B via the one-way "a=c m; b=a m" chain, never left in C
;;; across a gsbp call.
;;;
;;; NUMBER FORMAT (the other decisive finding this phase, see CLAUDE.md
;;; for the full derivation, including a retracted GTINDX-based
;;; approach - keep reading, this paragraph describes what actually
;;; works). The calculator's *displayed* X value is a normalized
;;; float, mantissa digits d1..d10 at nibbles 12..3 (d1=most
;;; significant) times 10^exponent (nibbles 0-2) - so typing "10"
;;; really stores as nibble12=1,nibble0=1, NOT a plain integer. Value
;;; arithmetic (C=C+1 M / C=C-1 M) needs the OTHER representation -
;;; a plain BCD integer, ones digit at nibble 3, matching field M's
;;; own p1=3 carry convention (so 6+4=10 correctly carries from
;;; nibble 3 into nibble 4). GTINDX (mainframe_cx.h) was tried as an
;;; OS-supplied float->plain-integer converter and is NOT that -
;;; isolated probing (typing a known X value, calling "gosub GTINDX",
;;; dumping register N's raw nibbles directly) showed its output does
;;; not correlate with X's actual value at all, so this module now
;;; does the conversion itself, both directions, via a fixed RCR
;;; rotation. RCR n's real semantics (verified directly against
;;; nutcpu.c's implementation) are new_C[i]=old_C[(n+i) mod 14] -
;;; solved for our two possible widths (LCLS/LCLX only ever see values
;;; 0-34, so always 1 or 2 digits): RCR 5 converts a 1-digit
;;; plain-integer to float (nibble 3 -> nibble 12, exponent nibbles
;;; already 0); RCR 6 converts a 2-digit one (nibble 4 -> nibble 12)
;;; plus explicitly setting exponent nibble 0 to 1 (rotation alone
;;; cannot produce a nonzero exponent digit, since the plain-integer
;;; input never had one). RCR 9 and RCR 8 are their exact inverses
;;; (14 minus the forward amount), used by LoadPlainFromX below to go
;;; the other way. Both directions were cross-checked against a raw
;;; register dump of *keystroke-settled* values (typed digits followed
;;; by a real ENTER^ key press, not read mid-digit-entry - an early
;;; version of this same probe skipped that and got misleading,
;;; not-yet-committed byte patterns): "1"/"2"/"6"/"9" all show
;;; nibble12=value with exponent nibbles all 0; "10"/"14"/"30" all
;;; show nibble12=tens digit, nibble11=ones digit, nibble0=1.

;;; REAL, CONFIRMED CALYPSI/FAT FINDING: the LAST .fat entry in a module
;;; never dispatches via XEQ ALPHA <name> ALPHA - it never even enters
;;; its own page (confirmed via modtool --summary showing its address
;;; correctly, plus a step-by-step trace showing regPC never touches
;;; that page at all). Confirmed by adding a trailing dummy FAT entry
;;; after it, which made the PREVIOUSLY-LAST entry start working. Fix:
;;; never let a real function be the last .fat entry - Padding below is
;;; a permanent, deliberate no-op placeholder for exactly this reason,
;;; not dead code to clean up.
              .section CODE
              .con    31
              .con    .fatsize FatEnd
              .fat    Header
              .fat    Lcls
              .fat    Lclx
              .fat    Lrst
              .fat    Padding
FatEnd:       .con    0,0

#include "mainframe.h"
;;; mainframe_cx.h is deliberately NOT included here - see the "LSTO/
;;; LRCL retired" section below for why: this module now makes no
;;; hardcoded-address gosub calls into any CX-mainframe-specific
;;; routine at all, on purpose, for real hardware portability.

              .name   "MULTIFOCAL PHASE2"
Header:       rtn

;;; Reads the FOCAL X register (abs reg 3) and converts its normalized
;;; float value into plain-integer form (ones digit at nibble 3,
;;; matching field M's own p1=3 carry convention) - the exact inverse
;;; of StoreFloatIntoX below, via the inverse RCR amount (14 minus the
;;; forward one). Handles exactly the two cases LCLS/LCLX ever take as
;;; input (0-9 or 10-34). Leaves the plain-integer value in C.M.
LoadPlainFromX:
              c=0     w
              pt=     0
              lc      3
              dadd=c             ; address = 3
              c=data             ; C = current float value of X
              pt=     0
              ?c#0    pt         ; exponent nibble 0 nonzero => 2-digit input
              goc     TwoDigitIn
              rcr     9          ; 1-digit: nibble 12 -> nibble 3 (undo RCR 5)
              rtn
TwoDigitIn:   rcr     8          ; 2-digit: nibble 12->4, nibble 11->3 (undo RCR 6)
              pt=     6
              c=0     pt         ; clear the stray nibble the old exponent digit rotated into
              rtn

;;; Converts B.M (a plain-integer value - ones digit at nibble 3, as
;;; LoadPlainFromX/field-M arithmetic produce) into normalized float
;;; form and writes it into X (abs reg 3). Handles exactly the two
;;; cases LCLS/LCLX ever produce (0-9 or 10-34) - see the file header
;;; comment for the RCR-rotation derivation. Value must be in B.M, not
;;; C, by the time this is gsbp-called (gsbp clobbers C on entry).
StoreFloatIntoX:
              c=0     w
              c=b     m          ; C = plain-integer value, all other fields clean (0)
              pt=     4
              ?c#0    pt         ; nibble 4 (tens digit) nonzero => 2-digit result
              goc     TwoDigit
              rcr     5          ; 1-digit: nibble 3 -> nibble 12, exponent stays 0
              goto    Store
TwoDigit:     rcr     6          ; 2-digit: nibble 4 -> nibble 12
              pt=     0
              lc      1          ; exponent nibble 0 = 1
Store:        a=c     w          ; stash the complete float value while DADD is set up
              c=0     w
              pt=     0
              lc      3
              dadd=c             ; address = 3
              c=a     w          ; restore the complete float value
              data=c
              rtn

              .name   "LCLS"
;;; X in: current logical stack size. X out: new size (current + 4), or
;;; X UNCHANGED if already at the depth ceiling (34 = 8 frames x 4 +
;;; header 2 - see Phase 1's recursion-depth-ceiling decision). Pure
;;; counter arithmetic - does NOT call RESZFL. MFSTK is allocated once,
;;; at its full maximum size, by the test harness's one-time keystroke
;;; setup (see the file header's "REDESIGN" note above); LCLS/LCLX only
;;; ever track how many of those pre-allocated registers are logically
;;; in use. A refused push/pop is intentionally silent (no ALPHA-based
;;; error message - see the file header on why touching ALPHA from
;;; MCODE is unsafe) - a caller can tell it was refused because X comes
;;; back unchanged instead of shifted by 4.
Lcls:         gsbp    LoadPlainFromX ; C.M = current size, plain-integer form
              setdec                ; needed for both C=C+1 M below and the ?A<C M bounds check (both are BCD-sensitive via nutcpu.c's shared flagdec gate)
              a=c     m             ; A.M = current size, stashed for the bounds check
              c=0     w
              pt=     4
              lc      3
              lc      4             ; C.M = 34 (plain form: nibble4=3, nibble3=4 - the depth ceiling)
              ?a<c    m             ; carry=1 iff current < 34 (room left to grow)
              gonc    LclsFull      ; carry clear means current >= 34 already - refuse the push
              c=a     m             ; C.M = current size again, for the real arithmetic
              c=c+1   m
              c=c+1   m
              c=c+1   m
              c=c+1   m             ; C.M = current+4
              sethex                ; restore hex mode before any further gsbp - gsbp's own mainframe relocation helper does its own address arithmetic in hex, and leaving decimal mode on corrupted its jump target (confirmed empirically: the very next gsbp landed on the wrong page)
              a=c     m
              b=a     m             ; B.M = new size
              gsbp    StoreFloatIntoX ; X = new size, normalized float form
              golong  ERR110        ; clean top-level completion - refreshes the display with X, goes idle (a bare "rtn" here left the display blank, confirmed empirically - see CLAUDE.md)
LclsFull:     sethex                ; same reasoning as above - restore hex mode before golong regardless of path taken
              golong  ERR110        ; X is left unchanged - the push was refused (already at the depth ceiling)

              .name   "LCLX"
;;; X in: current logical stack size. X out: new size (current - 4), or
;;; X UNCHANGED if already at the minimum (2 = header only, no frames
;;; left to pop). Pure counter arithmetic - does NOT call RESZFL, same
;;; reasoning as LCLS above. Same "refusal is silent, X unchanged"
;;; convention as LCLS.
Lclx:         gsbp    LoadPlainFromX ; C.M = current size, plain-integer form
              setdec                ; see LCLS's own comment on this - same reasoning
              a=c     m             ; A.M = current size, stashed for the bounds check
              c=0     w
              pt=     4
              lc      0
              lc      6             ; C.M = 6 (plain form: nibble4=0, nibble3=6) - the smallest size LCLX may act on (result would be 2, the empty-stack minimum)
              ?a<c    m             ; carry=1 iff current < 6 (already empty - nothing left to pop)
              goc     LclxEmpty     ; carry set means current < 6 already - refuse the pop
              c=a     m             ; C.M = current size again, for the real arithmetic
              c=c-1   m
              c=c-1   m
              c=c-1   m
              c=c-1   m             ; C.M = current-4
              sethex                ; see LCLS's own comment on this - same reasoning
              a=c     m
              b=a     m             ; B.M = new size
              gsbp    StoreFloatIntoX ; X = new size, normalized float form
              golong  ERR110        ; clean top-level completion, same reasoning as LCLS above
LclxEmpty:    sethex                ; same reasoning as LclsFull above
              golong  ERR110        ; X is left unchanged - the pop was refused (already empty)

;;; PHASE 3, then RETIRED (2026-09-05): local-variable read/write
;;; within the CURRENT (innermost) frame. Originally implemented as
;;; this module's own LSTO/LRCL functions (thin gosub wrappers around
;;; the real SAVEX/GETX primitives); removed once real HP-41CV+82180A
;;; testing confirmed a real, project-ending-for-this-approach bug -
;;; see CLAUDE.md's "Real HP-41CV+82180A boot config" section for the
;;; full story, condensed here:
;;;
;;; LSTO/LRCL's own bodies called SAVEX/GETX via `gosub` to a HARDCODED
;;; ADDRESS from mainframe_cx.h - correct only because those routines
;;; happen to live at that fixed address inside the CX's own built-in
;;; mainframe ROM. On a base HP-41C/CV with the real, port-pluggable
;;; 82180A module providing XM instead, those same routines live inside
;;; the 82180A's own ROM page - wherever the user happens to have it
;;; plugged in, not at the CX's fixed address. LCLS/LCLX/LRST (pure
;;; X-register arithmetic, no SAVEX/GETX calls at all) were never
;;; exposed to this and passed cleanly on first try against a real
;;; CV+82180A boot config; LSTO/LRCL failed identically every time
;;; (silently reading back stale values, not even erroring) - confirmed
;;; by reading the disassembled addresses directly: they landed in a
;;; page that's genuinely unpopulated on that configuration, so the
;;; `gosub` executed inert, uninitialized ROM instead of the real
;;; routine.
;;;
;;; Retiring LSTO/LRCL costs nothing functionally - by design, they
;;; never embedded a slot number or computed the target register
;;; themselves (that was always the calling FOCAL program's own job -
;;; see the calling convention below), so they were pure pass-throughs
;;; with no logic of their own beyond a corruption-bug workaround (see
;;; below) that a real catalog dispatch doesn't even need. The
;;; replacement calling convention is simpler, not just safer: use the
;;; real SAVEX/GETX directly, by name, exactly like CRFLD/SEEKPTA
;;; already have to be - real FOCAL-program-level keystrokes, dispatched
;;; through the OS's own catalog search (which resolves to whichever
;;; page the XM-providing module actually occupies, on ANY hardware
;;; configuration), never a MultiFOCAL-internal `gosub` to a fixed
;;; address:
;;;   "MFSTK"  <target register>  XEQ SEEKPTA  (real keystroke/program step)
;;;   <value>  XEQ SAVEX                        (or XEQ GETX to read)
;;; where <target register> = (current LCLS/LCLX size) - 3 + <slot 0-3>.
;;; Consecutive accesses to increasing slots need only ONE seek (GETX/
;;; SAVEX auto-advance the pointer - confirmed via a real sequential-
;;; write-then-sequential-read test, see CLAUDE.md); accessing a
;;; DIFFERENT, non-next register always needs a fresh SEEKPTA. This is
;;; a real, inherent HP-41 XM constraint (SEEKPTA is architecturally a
;;; FOCAL-program-level operation, not a subroutine primitive), not a
;;; gap unique to this design.
;;;
;;; MFSTK must still be allocated at size 35, not 34 - one MORE than
;;; the depth-ceiling arithmetic's own 34 (see Lcls/Lclx above). This
;;; is NOT a change to the depth ceiling itself (still 8 frames x 4 +
;;; header 2 = 34 *logical* registers) - it works around a separate,
;;; confirmed real HP-41CX bug: SEEKPTA to a file's own LAST register
;;; always fails with "END OF FL", regardless of file size (confirmed
;;; via pure keystrokes on both a 10-register and a 34-register file -
;;; this generalizes the Phase 2 finding that motivated MFSTK's
;;; original bump from size 1 to size 2, which was the same bug at the
;;; OTHER end of a degenerate 1-register file). Register 35 is
;;; permanent, deliberate padding - never written, never read - so
;;; that register 34 (the real deepest local-variable slot) is never
;;; the file's actual last register. This constraint is unrelated to
;;; LSTO/LRCL's own retirement and remains fully in force for direct
;;; SAVEX/GETX use.
;;;
;;; The corruption bug that drove LSTO's own asymmetric completion
;;; path (a real, confirmed bug: "gosub SAVEX" followed by "golong
;;; ERR110" corrupted the OS's file-directory lookup, breaking the
;;; caller's very next SEEKPTA - but real catalog-dispatched XEQ SAVEX
;;; was independently confirmed NOT to have this problem) is now
;;; entirely moot: a real XEQ SAVEX from the calling FOCAL program
;;; completes normally, with normal visible feedback, matching every
;;; other real X-Function - no workaround needed at all.
;;;
;;; PHASE 4: recovery from an orphaned frame after abrupt exit. LCLS/
;;; LCLX are pure X-in/X-out functions (see above) - the "current
;;; logical stack size" they thread through X has to be held somewhere
;;; persistent BETWEEN calls by the calling FOCAL program itself (its
;;; own body uses X for other things between push and pop), by
;;; convention in a dedicated FOCAL variable (recommended name: "MFSZ")
;;; that every subroutine STOs into right after LCLS and RCLs right
;;; before LCLX. If a subroutine that pushed a frame never reaches its
;;; matching LCLX - a FOCAL error abort (any op error halts the running
;;; program and returns to the keyboard), a GTO out of the subroutine,
;;; or a manual stop - MFSZ is left stuck at the elevated size. This
;;; doesn't corrupt anything (MFSTK's own contents are untouched - see
;;; Lcls/Lclx above, which never touch XM at all), but it permanently
;;; "loses" that frame's registers: every future LCLS silently builds
;;; on top of the stale size instead of reusing the orphaned space,
;;; eventually hitting the depth ceiling for no logical reason.
;;;
;;; Deliberately NOT auto-detected: this project's own history (the
;;; RESZFL/ERR110 hang, the SAVEX+ERR110 corruption bug - both above)
;;; is a repeated lesson that reaching into OS internals (here, that
;;; would mean patching the error-handling/abort path) is exactly where
;;; this project has hit its worst, hardest-to-diagnose bugs for a real
;;; HP-41. MultiFOCAL cannot tell "empty stack" apart from "orphaned
;;; stack" on its own, so it doesn't try - LRST is a manual recovery
;;; tool the FOCAL programmer calls at a known-safe point (e.g. the top
;;; of the main program, or an explicit menu/recovery routine), the
;;; same discipline Geir Isene's own coding standard already calls for
;;; ("every routine must return to the header... as its last step" -
;;; see CLAUDE.md's community-convention research) applied one level up
;;; to MultiFOCAL's own frame stack.
              .name   "LRST"
;;; No input read. Unconditionally writes X = 2 (the empty-stack
;;; minimum - header only, no frames) regardless of X's prior value.
;;; Touches no XM state at all - MFSTK's physical contents are never
;;; reclaimed or reinitialized, only the logical counter value the
;;; calling program threads through X (and is expected to STO into its
;;; own MFSZ variable right after this call) resets to empty. Safe to
;;; call any number of times, including when the stack is already
;;; empty (idempotent).
Lrst:         c=0     w
              pt=     3
              lc      2             ; C.M = 2 (plain-integer form: nibble3=2)
              a=c     m
              b=a     m             ; B.M = 2
              gsbp    StoreFloatIntoX ; X = 2, normalized float form
              golong  ERR110

;;; Deliberate, permanent no-op placeholder - see the FAT table comment
;;; above. Never remove without also removing the reason it's here:
;;; the true LAST .fat entry never dispatches, so this must always be
;;; whatever comes last, not any real callable function.
              .name   "MFPAD"
Padding:      rtn

              .section PollVectors
              .con    0
              .con    0
              .con    0
              .con    0
              .con    0
              .con    0
              .con    0
              .text   "A2WF"
