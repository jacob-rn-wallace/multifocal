;;; MultiFOCAL Phase 2 proof-of-concept: LCLS/LCLX frame enter/exit.
;;;
;;; SCOPE CUT (documented in CLAUDE.md): both LCLS and LCLX operate on a
;;; FIXED width of 4 registers per frame (not read from X, and no
;;; self-describing trailer register read back on pop) - the true
;;; variable-width, self-describing design turned out to need two
;;; independent values alive across OS calls at once; solvable, but out
;;; of scope for first proving the basic push/pop mechanism works.
;;;
;;; Register format (found the hard way - real HP-41 registers are NOT
;;; what the "field X" name suggests): a 14-nibble register is
;;; nibble13=sign, nibbles3-12="M" field = the 10 mantissa digits (ones
;;; digit at nibble 12, most significant at nibble 3), nibbles0-2="X"
;;; field = the exponent. The assembler's "field X" (used by e.g.
;;; "c=c+1 x") is that EXPONENT field, not the calculator's X-register
;;; value - a name collision between "assembler field X" and "FOCAL
;;; X-register" that cost real debugging time. To manipulate the actual
;;; numeric VALUE being passed to/from the FOCAL X-register, use field M
;;; (nibbles 3-12) and place single-digit literals at nibble 12, not 0.
;;; Field X (nibbles 0-2) is still the right, deliberate choice for
;;; DADD=C's address argument, which really does read exactly those
;;; three nibbles - that part was always correct.
;;;
;;; Calling convention across "gsbp" (page-relocatable local call,
;;; needed so this module works at whatever page it's loaded into):
;;; gsbp clobbers C while computing the jump target, confirmed by
;;; instruction trace - a value in C right before a gsbp call is gone by
;;; the callee's first instruction. B is untouched by it. Calypsi's
;;; "x=y f" syntax only has genuine one-way opcodes in one direction
;;; each per field group (B=A, C=B, A=C form a cycle); the reverse
;;; directions are realized via the corresponding EXCHANGE instead
;;; (ACEX/ABEX/BCEX), which also clobbers the source - fine for local
;;; scratch, not for carrying a value across a call. To pass a value
;;; into StoreBIntoX, it's relayed into B via the one-way "a=c m; b=a m"
;;; chain. B is never trusted to survive a "gosub" to a real CX routine
;;; (unverified either way) - values needed after one are re-derived
;;; fresh via GETX rather than assumed to have survived in a register.

              .section CODE
              .con    31
              .con    .fatsize FatEnd
              .fat    Header
              .fat    Lcls
              .fat    Lclx
FatEnd:       .con    0,0

#include "mainframe.h"
#include "mainframe_cx.h"
SEEKPTA      .equlab  0x3f35

              .name   "MULTIFOCAL PHASE2"
Header:       rtn

;;; Zero all 14 nibbles of C. Ends with pointer at nibble 0 (address
;;; field position - see SetValue12/address-setting call sites below).
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

;;; ALPHA (abs reg 5) = "MFSTK". Sets the address (5) FIRST, in its own
;;; throwaway C sequence, then builds the MFSTK pattern into a freshly-
;;; zeroed C afterward, so "data=c" writes the real pattern - not the
;;; address value - to the already-set destination.
SetAlphaMFSTK: gsbp   ZeroC
              lc      5
              dadd=c
              gsbp    ZeroC
              pt=     13
              lc      0
              lc      0
              lc      0
              lc      0
              lc      4
              lc      0xd
              lc      4
              lc      6
              lc      5
              lc      3
              lc      5
              lc      4
              lc      4
              lc      0xb
              data=c
              rtn

;;; Writes B.M (the value, mantissa field - see file header) into abs
;;; reg 3 (the FOCAL X register) as a clean positive integer: exponent
;;; (field X, nibbles 0-2) and sign (nibble 13) forced to 0, only the
;;; mantissa (field M) carries the real digits. The value must be in
;;; B.M, not C, by the time this is gsbp-called. A is pure local scratch
;;; here (via ACEX on field M) - nothing after this depends on A.
StoreBIntoX:  c=b     m
              a=c     m
              c=0     w
              pt=     0
              lc      3
              dadd=c             ; address = 3 (reads nibbles 0-2)
              c=0     x          ; re-clear the exponent field - dadd=c only READ nibbles 0-2, but they still hold "3,0,0" until explicitly cleared, which would otherwise leak into the number we're about to write
              c=a     m          ; restore the value into the mantissa field
              a=0     m
              data=c
              rtn

;;; C = current X register's raw 14-nibble value (abs reg 3). Safe to
;;; read C.M immediately after this returns and use it before any other
;;; gsbp/gosub call - "rtn" doesn't touch C.
LoadXIntoC:   c=0     w
              pt=     0
              lc      3
              dadd=c
              c=data
              rtn

;;; Seeking to abs-file register N of MFSTK is done inline at each call
;;; site (gsbp StoreBIntoX; gsbp SetAlphaMFSTK; gosub SEEKPTA) rather
;;; than through a shared helper - an earlier version factored this out
;;; as its own gsbp-called routine, adding one more level of call
;;; nesting; inlining it was tried as a fix for the still-unresolved bug
;;; documented in CLAUDE.md (didn't fix it, but kept as the leaner form).

;;; Sets C to the single-digit value N (0-9), correctly placed at
;;; nibble 12 (the mantissa's ones digit - see file header), all other
;;; nibbles zero. Use this instead of hand-rolling "gsbp ZeroC; lc N"
;;; with the pointer at the wrong position.
;;; (Assembled inline at each call site rather than as its own gsbp
;;; target, since it needs to leave its result in C for the caller's
;;; very next instruction - see ZeroC's own header note on why values
;;; can't be safely handed back across an extra call boundary here.)

              .name   "LCLS"
Lcls:
              ;; SCOPE CUT: file creation is NOT done here. Calling
              ;; CRFLD unconditionally on every LCLS hits a real DUP FL
              ;; error on the second and later calls (confirmed
              ;; empirically), and a real HP-41 error return does not
              ;; hand control back into the middle of this routine - it
              ;; goes straight to displaying the error and idling. A
              ;; production build needs a real existence check (or a
              ;; MEMORY LOST poll-vector hook that creates the file once
              ;; at cold-boot) before this can safely be idempotent; for
              ;; this proof, the caller is responsible for creating the
              ;; MFSTK file exactly once before the first LCLS.
              ;;
              ;; 1. Seek to header (reg 1), read current size T, compute T'=T+4.
              gsbp    ZeroC
              pt=     12
              lc      1
              a=c     m
              b=a     m             ; B.M = 1
              gsbp    StoreBIntoX
              gsbp    SetAlphaMFSTK
              gosub   SEEKPTA
              gosub   GETX
              gsbp    LoadXIntoC    ; C.M = T
              c=c+1   m
              c=c+1   m
              c=c+1   m
              c=c+1   m             ; C.M = T' = T+4
              a=c     m
              b=a     m             ; B.M = T'
              ;; 3. RESZFL(T') - grow the file by 4 registers.
              gsbp    StoreBIntoX
              gosub   RESZFL
              ;; 4. header (reg 1) = T'. Don't trust B/C survived the
              ;; RESZFL call above (unverified) - re-derive T' fresh:
              ;; header still holds the OLD T (not yet overwritten).
              gsbp    ZeroC
              pt=     12
              lc      1
              a=c     m
              b=a     m             ; B.M = 1
              gsbp    StoreBIntoX
              gsbp    SetAlphaMFSTK
              gosub   SEEKPTA
              gosub   GETX
              gsbp    LoadXIntoC    ; C.M = T (old, re-read)
              c=c+1   m
              c=c+1   m
              c=c+1   m
              c=c+1   m             ; C.M = T' (recomputed - cheap, avoids trusting a register across gosub)
              a=c     m
              b=a     m             ; B.M = T'
              gsbp    StoreBIntoX
              gosub   SAVEX         ; header (currently-positioned reg 1) = T'
              rtn

              .name   "LCLX"
Lclx:
              ;; 1. Seek to header (reg 1), read current size T, compute T'=T-4.
              gsbp    ZeroC
              pt=     12
              lc      1
              a=c     m
              b=a     m             ; B.M = 1
              gsbp    StoreBIntoX
              gsbp    SetAlphaMFSTK
              gosub   SEEKPTA
              gosub   GETX
              gsbp    LoadXIntoC    ; C.M = T
              c=c-1   m
              c=c-1   m
              c=c-1   m
              c=c-1   m             ; C.M = T' = T-4
              a=c     m
              b=a     m             ; B.M = T'
              ;; 2. RESZFL(T') - shrink the file by 4 registers, discarding the top frame.
              gsbp    StoreBIntoX
              gosub   RESZFL
              ;; 3. header (reg 1) = T'. Re-derive fresh, same reasoning as LCLS.
              gsbp    ZeroC
              pt=     12
              lc      1
              a=c     m
              b=a     m             ; B.M = 1
              gsbp    StoreBIntoX
              gsbp    SetAlphaMFSTK
              gosub   SEEKPTA
              gosub   GETX
              gsbp    LoadXIntoC    ; C.M = T (old, re-read)
              c=c-1   m
              c=c-1   m
              c=c-1   m
              c=c-1   m             ; C.M = T' (recomputed)
              a=c     m
              b=a     m             ; B.M = T'
              gsbp    StoreBIntoX
              gosub   SAVEX         ; header (currently-positioned reg 1) = T'
              rtn

              .section PollVectors
              .con    0
              .con    0
              .con    0
              .con    0
              .con    0
              .con    0
              .con    0
              .text   "A2WF"
