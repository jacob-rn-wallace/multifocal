/**
 * @file nut_rom_82180a.h
 * @brief Wires the real HP 82180A "Extended Functions/Extended Memory"
 *        module ROM into tabpage[]/typmod[] at a real HP-41 port page -
 *        the actual XM-providing hardware for a base HP-41C/CV, which
 *        has no XM built in (unlike the CX, whose XM lives in its own
 *        combined mainframe ROM - see nut_rom_cx.h).
 *
 * This is the real-hardware-target boot config this project has been
 * building toward since Phase 1 flagged "HP-41CV's XM compatibility is
 * genuinely unclear... needs direct verification" - see CLAUDE.md's
 * "Real HP-41CV+82180A boot config" section for the full story and the
 * user-sourced ROM's provenance.
 *
 * Source: the user's own sourced `~/soynut/roms/82180A.MOD` (a genuine
 * MOD1 container, confirmed via `modtool --summary` to declare XROM 25
 * "Extended Functions/Memory Module" with the expected real function
 * set - CRFLD, SEEKPTA, RCLPTA, GETX, SAVEX, GETRX, SAVERX, and the
 * rest this project already uses against the CX's built-in CXFUNS ROM)
 * - HP's copyrighted firmware, "bring your own", same convention as
 * every other ROM this project reads (see soynut/roms/README.md).
 * Converted by soynut's roms/mod_to_c.py (MOD1-aware, unlike
 * rom_to_c.py) into build/e82180a_rom.c (rom_82180a_p0[4096] - a
 * single page; the module's own MOD1 header declares Page=Any,
 * confirming it's genuinely port-pluggable, not fixed to one slot).
 *
 * Pair with soynut's own nut_boot() (firmware/emu41gcc_compat/
 * nut_rom.c) for the base OS - that function's own doc comment already
 * calls it "the base HP-41CV OS ROM" - NOT nut_boot_cx() (the CX
 * variant, which has no free port pages at 4-7 the way a base
 * CV does, and would never look for a plug-in XM module in the first
 * place since its own mainframe ROM already provides XM directly).
 *
 * **Page 4 (real Port 1) is deliberately NOT used - confirmed broken,
 * empirically, for ANY module, not just this one.** A disassembled
 * trace showed the base OS polls page 4 within the first handful of
 * cold-boot instructions (`GSUBNC 4000` at PC≈0x1AD, before almost
 * anything else runs) and, when ANY ROM occupies it (tried both this
 * real 82180A module and MultiFOCAL's own already-verified
 * `frames.mod` - both fail identically), the machine never reaches a
 * stable idle state - it loops repeatedly re-running its own stack-
 * clear ("MEMORY LOST" reset) sequence instead of settling, regardless
 * of module content (the module's own poll-vector code itself executes
 * cleanly and returns immediately every time - the instability is
 * entirely within the base OS's own post-poll logic, not the module).
 * This is independently consistent with `nut_boot_cx()`'s own comment
 * that "page 4 is not used by this configuration at all" for the CX
 * mainframe - two separate pieces of evidence agreeing that page 4 has
 * real, special-purpose early-cold-boot significance and is not a
 * safe general-purpose port for an ordinary module. **Page 5 (Port 2)
 * is used instead** - confirmed clean (`M E M O R Y   L O S T`) both
 * alone and alongside MultiFOCAL's own module at page 6 (Port 3).
 */
#ifndef MULTIFOCAL_NUT_ROM_82180A_H
#define MULTIFOCAL_NUT_ROM_82180A_H

/**
 * @brief Wire the 82180A module's ROM page into tabpage[]/typmod[] at
 *        page 5 - real HP-41 Port 2 (ports 1-4 map to pages 4-7,
 *        standard HP-41 hardware addressing; Port 1/page 4 is
 *        deliberately avoided - see the file header comment for why).
 *        Page=Any in the module's own MOD1 header confirms it
 *        genuinely doesn't care which port it sits in.
 *
 * Call once, after nut_boot() (nut_boot() doesn't touch page 5, so call
 * order relative to it doesn't actually matter, but "after" mirrors how
 * a real module would be plugged in after the calculator's own OS is
 * already resident - same convention as
 * nut_rom_wire_multifocal_test_module()).
 */
void nut_rom_wire_82180a(void);

#endif // MULTIFOCAL_NUT_ROM_82180A_H
