# Top-level build: assembles/links MultiFOCAL's own MCODE source (src/)
# into a MOD1 module file, then converts it to a C ROM array via
# soynut's existing roms/mod_to_c.py (read-only dependency - see
# CLAUDE.md's "Relationship to ~/soynut" section).
#
# Usage:
#   make mod         # src/*.s -> build/mftest.mod -> build/mftest_rom.c
#   make cx-roms     # soynut's XNUT0-2.ROM + CXFUNS0-1.ROM -> build/xnut_rom.c, build/cxfuns_rom.c
#                     # (needed by test/xm_probe and test/xm_trace - see CLAUDE.md's "Phase 2" section)
#   make test        # mod, then run the Phase 0 headless emulator loop test
#   make clean

CALYPSI := toolchain/calypsi-nut-5.18/bin
SOYNUT := ../soynut

.PHONY: mod cx-roms test clean
mod: build/mftest_rom.c
cx-roms: build/xnut_rom.c build/cxfuns_rom.c

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

test: mod
	$(MAKE) -C test run

clean:
	rm -rf build
	$(MAKE) -C test clean
