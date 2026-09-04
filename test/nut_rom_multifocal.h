/**
 * @file nut_rom_multifocal.h
 * @brief Wires the MultiFOCAL Phase 0 proof-of-concept module ROM into
 *        tabpage[]/typmod[] at the page this test harness assigns it.
 *
 * Source: src/mftest.s, assembled/linked by Calypsi (asnut/lnnut) into
 * build/mftest.mod, then converted by soynut's roms/mod_to_c.py into
 * build/mftest_rom.c (rom_mftest_p0[4096]) - see CLAUDE.md's "Phase 0
 * status" section. Mirrors soynut's own
 * firmware/emu41gcc_compat/nut_rom_hpil.c pattern for wiring an
 * optional module ROM in as a separate file from the base OS boot.
 *
 * Page 8 is an arbitrary free choice for this proof-of-concept only -
 * NOT a commitment to MultiFOCAL's eventual real port/page assignment
 * (that's a packaging decision for later, once the module is real).
 */
#ifndef MULTIFOCAL_NUT_ROM_MULTIFOCAL_H
#define MULTIFOCAL_NUT_ROM_MULTIFOCAL_H

/**
 * @brief Wire the MultiFOCAL Phase 0 test module's ROM page into
 *        tabpage[]/typmod[] at page 8.
 *
 * Call once, after nut_boot() (nut_boot() doesn't touch page 8, so call
 * order relative to it doesn't actually matter, but "after" mirrors how
 * a real module would be plugged in after the calculator's own OS is
 * already resident).
 */
void nut_rom_wire_multifocal_test_module(void);

#endif // MULTIFOCAL_NUT_ROM_MULTIFOCAL_H
