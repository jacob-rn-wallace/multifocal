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
;;; safely call GETX/SAVEX/RESZFL directly, but CANNOT safely call
;;; SEEKPTA/RCLPTA/CRFLD this way at all - only via a real keystroke/XEQ
;;; dispatch, which is why file creation happens once via the test
;;; harness's own keystrokes, never from inside LCLS/LCLX.
;;;
;;; This works because "current file" is persistent OS state that
;;; survives across separate XEQ invocations, not just within one call -
;;; as long as nothing ever re-selects a different file (which would
;;; need SEEKPTA/CRFLD), MFSTK stays "current" for RESZFL to act on.
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

              .section CODE
              .con    31
              .con    .fatsize FatEnd
              .fat    Header
              .fat    Lcls
              .fat    Lclx
FatEnd:       .con    0,0

#include "mainframe.h"
#include "mainframe_cx.h"

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
;;; X in: current stack size. X out: new size (current + 4). Grows the
;;; already-current MFSTK file by 4 registers via RESZFL - no seeking,
;;; no header, no ALPHA touch (see file header for why).
Lcls:         gsbp    LoadPlainFromX ; C.M = current size, plain-integer form
              setdec                ; C=C+1/-1 M do raw hex nibble arithmetic unless decimal mode is on (confirmed empirically - 9+1 gave 0xA, not a BCD-corrected 0-with-carry, until this was added)
              c=c+1   m
              c=c+1   m
              c=c+1   m
              c=c+1   m             ; C.M = current+4
              sethex                ; restore hex mode before any further gosub/gsbp - gsbp's own mainframe relocation helper does its own address arithmetic in hex, and leaving decimal mode on corrupted its jump target (confirmed empirically: the very next gsbp landed on the wrong page)
              a=c     m
              b=a     m             ; B.M = new size
              gsbp    StoreFloatIntoX ; X = new size, normalized float form
              gosub   RESZFL        ; grow the file to that size
              golong  ERR110        ; clean top-level completion - refreshes the display with X, goes idle (a bare "rtn" here left the display blank, confirmed empirically - see CLAUDE.md)

              .name   "LCLX"
;;; X in: current stack size. X out: new size (current - 4). Shrinks
;;; the already-current MFSTK file by 4 registers via RESZFL.
Lclx:         gsbp    LoadPlainFromX ; C.M = current size, plain-integer form
              setdec                ; see LCLS's own comment on this - same reasoning
              c=c-1   m
              c=c-1   m
              c=c-1   m
              c=c-1   m             ; C.M = current-4
              sethex                ; see LCLS's own comment on this - same reasoning
              a=c     m
              b=a     m             ; B.M = new size
              gsbp    StoreFloatIntoX ; X = new size, normalized float form
              gosub   RESZFL        ; shrink the file to that size
              golong  ERR110        ; clean top-level completion, same reasoning as LCLS above

              .section PollVectors
              .con    0
              .con    0
              .con    0
              .con    0
              .con    0
              .con    0
              .con    0
              .text   "A2WF"
