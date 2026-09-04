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

;;; Zero all 14 nibbles of C. Ends with pointer at nibble 0 (the address
;;; field position, used when this precedes an "lc <addr>; dadd=c").
ZeroC:        pt=     13
              lc      0
              lc      0
              lc      0
              lc      0
              lc      0
              lc      0
              lc      0
              lc      0
              lc      0
              lc      0
              lc      0
              lc      0
              lc      0
              lc      0
              pt=     0
              rtn

;;; Writes B.M (the value, mantissa field) into abs reg 3 (the FOCAL X
;;; register) as a clean positive integer - exponent and sign forced to
;;; 0, only the mantissa carries real digits. Value must be in B.M, not
;;; C, by the time this is gsbp-called (gsbp clobbers C on entry). A is
;;; pure local scratch here (via ACEX on field M).
StoreBIntoX:  c=b     m
              a=c     m
              c=0     w
              pt=     0
              lc      3
              dadd=c             ; address = 3 (reads nibbles 0-2)
              c=0     x          ; re-clear the exponent field - it still holds "3,0,0" after dadd=c read it
              c=a     m          ; restore the value into the mantissa field
              a=0     m
              data=c
              rtn

;;; C = current X register's raw 14-nibble value (abs reg 3). Safe to
;;; read C.M immediately after this returns - "rtn" doesn't touch C.
LoadXIntoC:   c=0     w
              pt=     0
              lc      3
              dadd=c
              c=data
              rtn

              .name   "LCLS"
;;; X in: current stack size. X out: new size (current + 4). Grows the
;;; already-current MFSTK file by 4 registers via RESZFL - no seeking,
;;; no header, no ALPHA touch (see file header for why).
Lcls:         gsbp    LoadXIntoC    ; C.M = current size
              c=c+1   m
              c=c+1   m
              c=c+1   m
              c=c+1   m             ; C.M = current+4
              a=c     m
              b=a     m             ; B.M = new size
              gsbp    StoreBIntoX   ; X = new size
              gosub   RESZFL        ; grow the file to that size
              golong  ERR110        ; clean top-level completion - refreshes the display with X, goes idle (a bare "rtn" here left the display blank, confirmed empirically - see CLAUDE.md)

              .name   "LCLX"
;;; X in: current stack size. X out: new size (current - 4). Shrinks
;;; the already-current MFSTK file by 4 registers via RESZFL.
Lclx:         gsbp    LoadXIntoC    ; C.M = current size
              c=c-1   m
              c=c-1   m
              c=c-1   m
              c=c-1   m             ; C.M = current-4
              a=c     m
              b=a     m             ; B.M = new size
              gsbp    StoreBIntoX   ; X = new size
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
