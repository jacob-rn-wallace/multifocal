# Research tool: disassemble real HP-41CX OS routines (base OS + the
# CXFUNS0/1 CX-only extensions, at pages 3-4) to verify calling
# conventions before writing MultiFOCAL's own MCODE against them - see
# CLAUDE.md's "Phase 2" section for how this was used.
#
# cxfuns_rom.c (generated from soynut's CXFUNS0.ROM/CXFUNS1.ROM, HP's
# copyrighted firmware) is gitignored, same "bring your own ROM" policy
# as soynut's own roms/rom_images.c - regenerate it with `make cxfuns`.
#
# Usage:
#   make cxfuns                  # (re)generate cxfuns_rom.c from soynut's ROMs
#   make cx_disasm               # build the disassembler
#   ./build/cx_disasm 3e36 20    # disassemble 20 instructions from 0x3E36

CC ?= cc
SOYNUT := ../../soynut
BUILD := build

.PHONY: cxfuns cx_disasm clean
cx_disasm: $(BUILD)/cx_disasm

cxfuns:
	python3 $(SOYNUT)/roms/rom_to_c.py $(SOYNUT)/roms/CXFUNS0.ROM $(SOYNUT)/roms/CXFUNS1.ROM > cxfuns_rom.c

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/desas41.o: $(SOYNUT)/emu41gcc/desas41.c | $(BUILD)
	$(CC) -std=gnu11 -include desas_proto.h -c -o $@ $<

$(BUILD)/rom_images.o: $(SOYNUT)/roms/rom_images.c | $(BUILD)
	$(CC) -std=gnu11 -c -o $@ $<

$(BUILD)/cxfuns_rom.o: cxfuns_rom.c | $(BUILD)
	$(CC) -std=gnu11 -c -o $@ $<

$(BUILD)/cx_disasm.o: cx_disasm.c | $(BUILD)
	$(CC) -std=gnu11 -include desas_proto.h -c -o $@ $<

$(BUILD)/cx_disasm: $(BUILD)/cx_disasm.o $(BUILD)/desas41.o $(BUILD)/rom_images.o $(BUILD)/cxfuns_rom.o
	$(CC) -o $@ $^

clean:
	rm -rf $(BUILD) cxfuns_rom.c
