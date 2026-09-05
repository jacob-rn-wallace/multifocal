# Top-level build: assembles/links MultiFOCAL's own MCODE source (src/)
# into a MOD1 module file, then converts it to a C ROM array via
# soynut's existing roms/mod_to_c.py (read-only dependency - see
# CLAUDE.md's "Relationship to ~/soynut" section).
#
# Usage:
#   make mod         # src/*.s -> build/mftest.mod -> build/mftest_rom.c
#   make cx-roms     # soynut's XNUT0-2.ROM + CXFUNS0-1.ROM -> build/xnut_rom.c, build/cxfuns_rom.c
#                     # (needed by test/xm_probe and test/xm_trace - see CLAUDE.md's "Phase 2" section)
#   make cv82180a-roms  # 82180A.MOD (real HP 82180A Extended Functions/Extended Memory
#                     # module) -> build/e82180a_rom.c - paired with soynut's own
#                     # nut_boot()/rom_images.c (plain NUT0-2 base OS, already built for
#                     # phase0_loop_test - reused as-is, not duplicated) for the real-
#                     # hardware-target boot config, see CLAUDE.md's "Real HP-41CV+82180A
#                     # boot config" section
#   make test        # mod, then run the Phase 0 headless emulator loop test
#   make clean

CALYPSI := toolchain/calypsi-nut-5.18/bin
SOYNUT := ../soynut

.PHONY: mod cx-roms cv82180a-roms test clean
mod: build/mftest_rom.c
cx-roms: build/xnut_rom.c build/cxfuns_rom.c build/timer_rom.c
cv82180a-roms: build/e82180a_rom.c

build/mftest.o: src/mftest.s
	mkdir -p build
	$(CALYPSI)/asnut src/mftest.s -I toolchain/calypsi-nut-5.18/include -o build/mftest.o

build/mftest.mod: build/mftest.o src/plugin4k.scm src/mftest.moddesc
	$(CALYPSI)/lnnut build/mftest.o src/plugin4k.scm src/mftest.moddesc -o build/mftest.mod

build/mftest_rom.c: build/mftest.mod
	python3 $(SOYNUT)/roms/mod_to_c.py build/mftest.mod > build/mftest_rom.c

build/xnut_rom.c:
	mkdir -p build
	python3 $(SOYNUT)/roms/rom_to_c.py $(SOYNUT)/roms/XNUT0.ROM $(SOYNUT)/roms/XNUT1.ROM $(SOYNUT)/roms/XNUT2.ROM > build/xnut_rom.c

build/cxfuns_rom.c:
	mkdir -p build
	python3 $(SOYNUT)/roms/rom_to_c.py $(SOYNUT)/roms/CXFUNS0.ROM $(SOYNUT)/roms/CXFUNS1.ROM > build/cxfuns_rom.c

build/timer_rom.c:
	mkdir -p build
	python3 $(SOYNUT)/roms/rom_to_c.py $(SOYNUT)/roms/TIMER.ROM > build/timer_rom.c

# 82180A.MOD is a MOD1 container (like HPIL.MOD), not raw .ROM pages -
# mod_to_c.py understands that format, rom_to_c.py doesn't.
build/e82180a_rom.c:
	mkdir -p build
	python3 $(SOYNUT)/roms/mod_to_c.py $(SOYNUT)/roms/82180A.MOD > build/e82180a_rom.c

test: mod
	$(MAKE) -C test run

clean:
	rm -rf build
	$(MAKE) -C test clean
