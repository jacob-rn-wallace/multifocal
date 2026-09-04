;;; ************************************************************
;;;
;;; MultiFOCAL Phase 0 proof-of-concept module.
;;;
;;; Not part of MultiFOCAL's real function set - LCLS/LCLX/LSTO/LRCL
;;; naming and behavior are still open decisions (see CLAUDE.md). This
;;; exists solely to prove the assemble -> link -> MOD1 -> emulator ->
;;; XEQ-by-name loop works end to end, using a throwaway FAT entry name
;;; ("MFTEST") that carries no naming-convention commitment.
;;;
;;; ************************************************************

              .section CODE
              .con    30            ; XROM number (placeholder, unassigned)
              .con    .fatsize FatEnd ; number of entry points

              .fat    Header        ; ROM header
              .fat    Mftest
FatEnd:       .con    0,0

#include "mainframe.h"

;;; ROM header - if executed directly, just return.
              .name   "MULTIFOCAL PHASE0"
Header:       rtn

;;; MFTEST: displays a message proving XEQ-by-name found this module's
;;; FAT entry and ran real MCODE out of it.
              .name   "MFTEST"
Mftest:       gosub   ERRSUB
              gosub   CLLCDE
              gosub   MESSL
              .messl  "MFTEST WORKS"
              st=1    8
              gosub   MSG105
              golong  ERR110

              .section PollVectors
              .con    0             ; Pause
              .con    0             ; Running
              .con    0             ; Wake w/o key
              .con    0             ; Powoff
              .con    0             ; I/O
              .con    0             ; Deep wake-up
              .con    0             ; Memory lost
              .text   "A1WM"        ; Identifier (arbitrary, in reverse)
