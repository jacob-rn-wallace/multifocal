/**
 * @file hp41_raw_keys.h
 * @brief Raw HP-41 keycode injection, bypassing hp41_key_bridge.c's
 *        restricted tabcode[]/named_keys[] tables entirely.
 *
 * `dokey()` in nutcpu.c pushes `keybuffer[0]` straight into `regK` with
 * no further translation - the codes hp41_key_bridge.c's tables carry
 * are genuine HP-41 hardware key-matrix codes, not an emulator-only
 * abstraction. Real physical keys with no ASCII equivalent and no
 * `named_keys[]` entry (STO, RCL, GTO, LBL, SIN, ...) are therefore
 * still reachable - just not through hp41_key_bridge.c's own, smaller,
 * purpose-built table (soynut's own file, read-only for this project -
 * see CLAUDE.md's "Relationship to ~/soynut"). This header is
 * MultiFOCAL's own, local addition; it never modifies soynut.
 *
 * Two codes are confirmed here, found by direct brute-force probing
 * against the real ROM (see CLAUDE.md's "Real HP-41 keycodes found"
 * section for the full methodology and a real pitfall hit along the
 * way - nut_boot_cx() does not reset espaceRAM/keybuffer/flagKey, so
 * a loop of many nut_boot_cx() calls in one process silently
 * accumulates cross-iteration contamination; every probe here uses one
 * process per candidate to avoid it):
 *
 *   HP41_KEY_STO = 0x52 - confirmed via the real "S T O  _ _" prompt
 *   display and a clean register round-trip (typing a value, this key,
 *   then a 2-digit register number correctly stores it - single-digit
 *   register entry like "5" alone does NOT commit, the prompt stays
 *   open; always use 2 digits, e.g. "05").
 *
 *   HP41_KEY_RCL = 0x82 - confirmed the same way, paired with
 *   HP41_KEY_STO in a round-trip test across 3 independent value/
 *   register combinations (42/07, 99/12, 5/03), all correct.
 *
 * GTO's keycode was NOT found this pass, despite real effort - unlike
 * STO/RCL/XEQ, it does not spell its own name as a display prompt, and
 * a program-branch test (skip a line via "GTO .004") was inconclusive:
 * several non-GTO candidates (including the confirmed STO/RCL
 * themselves) share similar argument-consuming display side effects,
 * so the test wasn't uniquely diagnostic. LBL (SHIFT+GTO, matching the
 * real BST=SHIFT+SST pattern) is consequently also still unknown.
 */
#ifndef MULTIFOCAL_HP41_RAW_KEYS_H
#define MULTIFOCAL_HP41_RAW_KEYS_H

#define HP41_KEY_STO 0x52
#define HP41_KEY_RCL 0x82

/*
 * No shared injection function is provided here - this project's own
 * test/ *.c convention is small, per-file static helpers (type_byte(),
 * pump(), etc., each copy-pasted, not shared via a .c file), and raw
 * injection follows the same pattern. Add this to any test that needs
 * it, alongside its own type_byte()/pump()/asleep_state:
 *
 *   static int inject_raw(unsigned char code) {
 *       if (asleep_state) { flagKey = 0; regPC = 0; asleep_state = false; }
 *       if (lgkeybuf < 8) keybuffer[lgkeybuf++] = (char)code;
 *       int ret = pump(200000);
 *       if (ret == 1) asleep_state = true;
 *       return ret;
 *   }
 *
 * Real HP-41 STO/RCL convention, confirmed empirically (not assumed):
 * both always take a full 2-digit register number (e.g. "05", not
 * "5") - a single digit leaves the "S T O  _ _" / "R C L  _ _" prompt
 * open without committing.
 */

#endif /* MULTIFOCAL_HP41_RAW_KEYS_H */
