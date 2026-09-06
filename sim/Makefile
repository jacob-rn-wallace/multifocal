# MultiFOCAL's own interactive host simulator - the real, hand-usable
# counterpart to test/'s automated keystroke-injection tests. See
# mf_sim_main.c's own header for what this is and how it differs from
# soynut's own sim/sim_main.c (exactly one addition: wiring in the real
# 82180A module + MultiFOCAL's own frames.mod after nut_boot()).
#
# Modeled directly on soynut's own sim/Makefile (see that file) and this
# repo's own test/Makefile - reuses soynut's sim_*.c/firmware/*.c/
# emu41gcc/*.c/roms/*.c sources directly by path (never copied, per this
# project's standing "nothing under ~/soynut is built or modified"
# convention), builds only mf_sim_main.c (this directory's own file) and
# this repo's own test/nut_rom_82180a.c wiring helper natively.
#
# Requires SDL2 (macOS: brew install sdl2), soynut/roms/rom_images.c to
# already exist (soynut's own "bring your own ROM" requirement), and
# both build/frames_rom.c and build/e82180a_rom.c to already exist (see
# the top-level Makefile's `mod` and `cv82180a-roms` targets).
#
# Usage:
#   make -C sim           # build sim/build/multifocal_sim
#   make -C sim run       # build, then run it (continuous-memory file
#                          # lands at sim/soynut_sim_persist.bin, same
#                          # fixed name soynut's own sim uses - a
#                          # separate file, since it's a separate
#                          # directory)
#   make -C sim clean

CC ?= cc
SOYNUT := ../../soynut
ROOT := $(SOYNUT)
BUILD := build

STD := -std=gnu11
STRICT := -Wall -Wextra -Wpedantic -Werror -UNDEBUG
INC := -I$(ROOT)/emu41gcc -I$(ROOT)/firmware/emu41gcc_compat -I$(ROOT)/firmware -I$(ROOT)/font-tables -I$(ROOT)/ilvideo-native/core -I$(SOYNUT)/sim -I../test
NUT_STUBS_INC := -include $(ROOT)/firmware/emu41gcc_compat/nut_stubs.h

SDL_CFLAGS := $(shell pkg-config --cflags sdl2)
SDL_LIBS := $(shell pkg-config --libs sdl2)

.PHONY: all run clean mod cv82180a-roms
all: $(BUILD)/multifocal_sim

mod:
	$(MAKE) -C .. mod

cv82180a-roms:
	$(MAKE) -C .. cv82180a-roms

$(BUILD):
	mkdir -p $(BUILD)

# --- Vendored (emu41gcc) and generated (roms/, font-tables/) sources:
# not ours, compiled leniently - only the flags actually required to
# build them at all. Same split rationale as test/Makefile and soynut's
# own sim/Makefile: strict warnings must never leak onto code we don't
# own and can't fix. ---

$(BUILD)/nutcpu.o: $(ROOT)/emu41gcc/nutcpu.c | $(BUILD)
	$(CC) $(STD) -fcommon $(INC) $(NUT_STUBS_INC) -c -o $@ $<

$(BUILD)/display.o: $(ROOT)/emu41gcc/display.c | $(BUILD)
	$(CC) $(STD) -fcommon $(INC) -c -o $@ $<

$(BUILD)/rom_images.o: $(ROOT)/roms/rom_images.c | $(BUILD)
	$(CC) $(STD) $(INC) -c -o $@ $<

$(BUILD)/hp41_display_tables.o: $(ROOT)/font-tables/hp41_display_tables.c | $(BUILD)
	$(CC) $(STD) $(INC) -c -o $@ $<

$(BUILD)/hp41_elite_font_table.o: $(ROOT)/font-tables/hp41_elite_font_table.c | $(BUILD)
	$(CC) $(STD) $(INC) -c -o $@ $<

# --- Reused firmware/ compat shims and pure-logic bridge files -
# unmodified from soynut/firmware/, same objects test/Makefile and
# soynut's own sim/Makefile already build natively. ---

$(BUILD)/nut_stubs.o: $(ROOT)/firmware/emu41gcc_compat/nut_stubs.c | $(BUILD)
	$(CC) $(STD) $(STRICT) $(INC) -c -o $@ $<

$(BUILD)/nut_globals.o: $(ROOT)/firmware/emu41gcc_compat/nut_globals.c | $(BUILD)
	$(CC) $(STD) -fcommon $(STRICT) $(INC) -c -o $@ $<

$(BUILD)/nut_rom.o: $(ROOT)/firmware/emu41gcc_compat/nut_rom.c | $(BUILD)
	$(CC) $(STD) -fcommon $(STRICT) $(INC) -c -o $@ $<

$(BUILD)/hp41_key_bridge.o: $(ROOT)/firmware/hp41_key_bridge.c | $(BUILD)
	$(CC) $(STD) -fcommon $(STRICT) $(INC) -c -o $@ $<

$(BUILD)/hp41_key_hold_bridge.o: $(ROOT)/firmware/hp41_key_hold_bridge.c | $(BUILD)
	$(CC) $(STD) -fcommon $(STRICT) $(INC) -c -o $@ $<

$(BUILD)/hp41_display_bridge.o: $(ROOT)/firmware/hp41_display_bridge.c | $(BUILD)
	$(CC) $(STD) $(STRICT) $(INC) -c -o $@ $<

$(BUILD)/hp41_elite_display_bridge.o: $(ROOT)/firmware/hp41_elite_display_bridge.c | $(BUILD)
	$(CC) $(STD) -fcommon $(STRICT) $(INC) -c -o $@ $<

$(BUILD)/hp41_persist_state.o: $(ROOT)/firmware/hp41_persist_state.c | $(BUILD)
	$(CC) $(STD) -fcommon $(STRICT) $(INC) -c -o $@ $<

$(BUILD)/hp41_ascii_decode.o: $(ROOT)/firmware/hp41_ascii_decode.c | $(BUILD)
	$(CC) $(STD) $(STRICT) $(INC) -c -o $@ $<

$(BUILD)/hp41_register_decode.o: $(ROOT)/firmware/hp41_register_decode.c | $(BUILD)
	$(CC) $(STD) $(STRICT) $(INC) -c -o $@ $<

$(BUILD)/hp41_hpil_controller.o: $(ROOT)/firmware/hp41_hpil_controller.c | $(BUILD)
	$(CC) $(STD) $(STRICT) $(INC) -c -o $@ $<

$(BUILD)/hp41_hpil_video_bridge.o: $(ROOT)/firmware/hp41_hpil_video_bridge.c | $(BUILD)
	$(CC) $(STD) $(STRICT) $(INC) -c -o $@ $<

# --- ilvideo-native core/: not ours, compiled leniently. ---

$(BUILD)/hpil_device.o: $(ROOT)/ilvideo-native/core/hpil_device.c | $(BUILD)
	$(CC) $(STD) $(INC) -c -o $@ $<

$(BUILD)/ilvideo_term.o: $(ROOT)/ilvideo-native/core/ilvideo_term.c | $(BUILD)
	$(CC) $(STD) $(INC) -c -o $@ $<

$(BUILD)/ilvideo_device.o: $(ROOT)/ilvideo-native/core/ilvideo_device.c | $(BUILD)
	$(CC) $(STD) $(INC) -c -o $@ $<

# --- soynut's own sim/*.c - reused directly by path, never copied. ---

$(BUILD)/sim_display.o: $(SOYNUT)/sim/sim_display.c | $(BUILD)
	$(CC) $(STD) $(STRICT) $(INC) $(SDL_CFLAGS) -c -o $@ $<

$(BUILD)/sim_persist_file.o: $(SOYNUT)/sim/sim_persist_file.c | $(BUILD)
	$(CC) $(STD) $(STRICT) $(INC) -c -o $@ $<

$(BUILD)/sim_keyboard.o: $(SOYNUT)/sim/sim_keyboard.c | $(BUILD)
	$(CC) $(STD) $(STRICT) $(INC) $(SDL_CFLAGS) -c -o $@ $<

$(BUILD)/sim_clock.o: $(SOYNUT)/sim/sim_clock.c | $(BUILD)
	$(CC) $(STD) $(STRICT) $(INC) $(SDL_CFLAGS) -c -o $@ $<

$(BUILD)/sim_pty.o: $(SOYNUT)/sim/sim_pty.c | $(BUILD)
	$(CC) $(STD) $(STRICT) $(INC) -c -o $@ $<

# --- This project's own sources. ---

$(BUILD)/mf_sim_main.o: mf_sim_main.c | $(BUILD)
	$(CC) $(STD) -fcommon $(STRICT) $(INC) $(SDL_CFLAGS) -c -o $@ $<

$(BUILD)/nut_rom_82180a.o: ../test/nut_rom_82180a.c | $(BUILD)
	$(CC) $(STD) -fcommon $(INC) -c -o $@ $<

$(BUILD)/e82180a_rom.o: ../build/e82180a_rom.c | $(BUILD)
	$(CC) $(STD) $(INC) -c -o $@ $<

$(BUILD)/frames_rom.o: ../build/frames_rom.c | $(BUILD)
	$(CC) $(STD) $(INC) -c -o $@ $<

# --- Link: everything above, plus SDL2's libs. ---

$(BUILD)/multifocal_sim: $(BUILD)/mf_sim_main.o $(BUILD)/sim_display.o $(BUILD)/sim_persist_file.o \
                      $(BUILD)/sim_keyboard.o $(BUILD)/sim_clock.o $(BUILD)/sim_pty.o \
                      $(BUILD)/nutcpu.o $(BUILD)/display.o \
                      $(BUILD)/nut_stubs.o $(BUILD)/nut_globals.o $(BUILD)/nut_rom.o \
                      $(BUILD)/hp41_key_bridge.o $(BUILD)/hp41_key_hold_bridge.o \
                      $(BUILD)/hp41_display_bridge.o $(BUILD)/hp41_elite_display_bridge.o \
                      $(BUILD)/hp41_ascii_decode.o $(BUILD)/hp41_register_decode.o \
                      $(BUILD)/hp41_persist_state.o \
                      $(BUILD)/hp41_hpil_controller.o $(BUILD)/hp41_hpil_video_bridge.o \
                      $(BUILD)/hpil_device.o $(BUILD)/ilvideo_term.o $(BUILD)/ilvideo_device.o \
                      $(BUILD)/rom_images.o $(BUILD)/hp41_display_tables.o $(BUILD)/hp41_elite_font_table.o \
                      $(BUILD)/nut_rom_82180a.o $(BUILD)/e82180a_rom.o $(BUILD)/frames_rom.o
	$(CC) -o $@ $^ $(SDL_LIBS)

run: all
	./$(BUILD)/multifocal_sim

clean:
	rm -rf $(BUILD)
