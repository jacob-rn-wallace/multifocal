/**
 * @file nut_rom_cx.h
 * @brief Boots a genuine HP-41CX configuration: the CX-variant base OS
 *        (XNUT0-2, not soynut's own nut_boot()'s plain NUT0-2) at pages
 *        0-2, plus the CX-only extension ROM (CXFUNS0-1 - Extended
 *        Functions/Memory, Time, etc.) at pages 3-4.
 *
 * **Do not combine soynut's own nut_boot() (plain NUT0-2, the C/CV
 * variant) with CXFUNS0-1.** Confirmed empirically (see CLAUDE.md's
 * "Phase 2" section) that NUT0-2.ROM and XNUT0-2.ROM are genuinely
 * different ROM dumps (`cmp` shows real byte differences) - the plain
 * variant's own OS code never looks for CX extension pages at all, so
 * `XEQ ALPHA CRFLD ALPHA` (or any other CX-only function) comes back
 * NONEXISTENT even with CXFUNS0-1 correctly wired at pages 3-4. This
 * mirrors the real hardware exactly: a genuine HP-41CX's mainframe ROM
 * is a combined 5-page unit built to recognize its own extension pages,
 * not a plain C/CV base OS with extra pages bolted on.
 *
 * Source: soynut's roms/XNUT0.ROM / XNUT1.ROM / XNUT2.ROM and
 * roms/CXFUNS0.ROM / CXFUNS1.ROM (HP's copyrighted firmware, "bring your
 * own" - see soynut/roms/README.md), converted by soynut's
 * roms/rom_to_c.py into build/xnut_rom.c and build/cxfuns_rom.c.
 */
#ifndef MULTIFOCAL_NUT_ROM_CX_H
#define MULTIFOCAL_NUT_ROM_CX_H

/**
 * @brief Wire a genuine CX configuration (XNUT0-2 + CXFUNS0-1 at pages
 *        0-4) into tabpage[]/typmod[], and reset CPU state to Nut's
 *        documented cold-start values - the CX equivalent of soynut's
 *        own nut_rom.c/nut_boot(), which this replaces rather than
 *        supplements for this test.
 */
void nut_boot_cx(void);

#endif // MULTIFOCAL_NUT_ROM_CX_H
