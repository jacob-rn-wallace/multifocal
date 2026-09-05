# Research tool: disassemble real HP-41CX OS routines (base OS + the
# CXFUNS0/1 CX-only extensions, at pages 3-4) to verify calling
# conventions before writing MultiFOCAL's own MCODE against them - see
# CLAUDE.md's "Phase 2" section for how this was used.
#
# xnut_rom.c/cxfuns_rom.c (generated from soynut's XNUT0-2.ROM/
# CXFUNS0-1.ROM, HP's copyrighted firmware) are gitignored, same
# "bring your own ROM" policy as soynut's own roms/rom_images.c -
# regenerate with `make xnut cxfuns`.
#
# FIXED (Phase 3): this tool used to link soynut's plain rom_images.c
# (rom_nut0/1/2 - the C/CV base OS) for pages 0-2, which is NOT a valid
# real HP-41CX configuration (see CLAUDE.md's Phase 2 "SAVERX/GETRX...
# was built on disassembling the wrong ROM" retraction) - any
# disassembly here that crossed into pages 0-2 before this fix was
# analyzing bytes that don't actually run in a real CX boot. Now uses
# rom_xnut0/1/2 (XNUT0-2.ROM, the real CX base OS), matching
# test/nut_rom_cx.c's own boot config exactly.
#
# Usage:
#   make xnut cxfuns              # (re)generate xnut_rom.c/cxfuns_rom.c from soynut's ROMs
#   make cx_disasm                # build the disassembler
#   ./build/cx_disasm 3e36 20     # disassemble 20 instructions from 0x3E36

CC ?= cc
SOYNUT := ../../soynut
BUILD := build

.PHONY: xnut cxfuns cx_disasm clean
cx_disasm: $(BUILD)/cx_disasm

xnut:
	python3 $(SOYNUT)/roms/rom_to_c.py $(SOYNUT)/roms/XNUT0.ROM $(SOYNUT)/roms/XNUT1.ROM $(SOYNUT)/roms/XNUT2.ROM > xnut_rom.c

cxfuns:
	python3 $(SOYNUT)/roms/rom_to_c.py $(SOYNUT)/roms/CXFUNS0.ROM $(SOYNUT)/roms/CXFUNS1.ROM > cxfuns_rom.c

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/desas41.o: $(SOYNUT)/emu41gcc/desas41.c | $(BUILD)
	$(CC) -std=gnu11 -include desas_proto.h -c -o $@ $<

$(BUILD)/xnut_rom.o: xnut_rom.c | $(BUILD)
	$(CC) -std=gnu11 -c -o $@ $<

$(BUILD)/cxfuns_rom.o: cxfuns_rom.c | $(BUILD)
	$(CC) -std=gnu11 -c -o $@ $<

$(BUILD)/cx_disasm.o: cx_disasm.c | $(BUILD)
	$(CC) -std=gnu11 -include desas_proto.h -c -o $@ $<

$(BUILD)/cx_disasm: $(BUILD)/cx_disasm.o $(BUILD)/desas41.o $(BUILD)/xnut_rom.o $(BUILD)/cxfuns_rom.o
	$(CC) -o $@ $^

clean:
	rm -rf $(BUILD) xnut_rom.c cxfuns_rom.c
