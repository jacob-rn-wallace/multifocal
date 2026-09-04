# Top-level build: assembles/links MultiFOCAL's own MCODE source (src/)
# into a MOD1 module file, then converts it to a C ROM array via
# soynut's existing roms/mod_to_c.py (read-only dependency - see
# CLAUDE.md's "Relationship to ~/soynut" section).
#
# Usage:
#   make mod         # src/*.s -> build/mftest.mod -> build/mftest_rom.c
#   make test        # mod, then run the Phase 0 headless emulator loop test
#   make clean

CALYPSI := toolchain/calypsi-nut-5.18/bin
SOYNUT := ../soynut

.PHONY: mod test clean
mod: build/mftest_rom.c

build/mftest.o: src/mftest.s
	mkdir -p build
	$(CALYPSI)/asnut src/mftest.s -I toolchain/calypsi-nut-5.18/include -o build/mftest.o

build/mftest.mod: build/mftest.o src/plugin4k.scm src/mftest.moddesc
	$(CALYPSI)/lnnut build/mftest.o src/plugin4k.scm src/mftest.moddesc -o build/mftest.mod

build/mftest_rom.c: build/mftest.mod
	python3 $(SOYNUT)/roms/mod_to_c.py build/mftest.mod > build/mftest_rom.c

test: mod
	$(MAKE) -C test run

clean:
	rm -rf build
	$(MAKE) -C test clean
