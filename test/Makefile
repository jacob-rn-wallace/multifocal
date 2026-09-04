# Headless Phase 0 loop test - see phase0_loop_test.c.
#
# Reuses soynut's real Nut CPU core (emu41gcc), its firmware compat
# shims, and its already-built base OS ROM (roms/rom_images.c) as a
# read-only dependency (../../soynut, a sibling directory) - by
# explicit user decision, soynut's own repo is never modified or built
# from here. MultiFOCAL's own module ROM (build/mftest_rom.c, generated
# from src/mftest.s - see the top-level Makefile) is this project's own
# file.
#
# Requires ../../soynut/roms/rom_images.c to already exist (soynut's own
# "bring your own ROM" requirement - see soynut/roms/README.md) and
# build/mftest_rom.c to already exist (see top-level Makefile's `mod`
# target).
#
# Usage:
#   make -C test run       # build (assembling src/mftest.s first if
#                           # needed) and run the Phase 0 loop test

CC ?= cc
SOYNUT := ../../soynut
BUILD := build

STD := -std=gnu11
INC := -I$(SOYNUT)/emu41gcc -I$(SOYNUT)/firmware/emu41gcc_compat -I$(SOYNUT)/firmware -I.
NUT_STUBS_INC := -include $(SOYNUT)/firmware/emu41gcc_compat/nut_stubs.h

.PHONY: all run clean mod cx-roms xm_probe xm_trace
all: $(BUILD)/phase0_loop_test

xm_probe: cx-roms $(BUILD)/xm_probe_test
	./$(BUILD)/xm_probe_test

xm_trace: cx-roms $(BUILD)/xm_trace_test
	./$(BUILD)/xm_trace_test

mod:
	$(MAKE) -C .. mod

cx-roms:
	$(MAKE) -C .. cx-roms

$(BUILD):
	mkdir -p $(BUILD)

# --- soynut's vendored/generated sources, read-only, compiled leniently
# (matches soynut's own tests/Makefile treatment of these exact files). ---

$(BUILD)/nutcpu.o: $(SOYNUT)/emu41gcc/nutcpu.c | $(BUILD)
	$(CC) $(STD) -fcommon $(INC) $(NUT_STUBS_INC) -c -o $@ $<

$(BUILD)/display.o: $(SOYNUT)/emu41gcc/display.c | $(BUILD)
	$(CC) $(STD) -fcommon $(INC) -c -o $@ $<

$(BUILD)/rom_images.o: $(SOYNUT)/roms/rom_images.c | $(BUILD)
	$(CC) $(STD) $(INC) -c -o $@ $<

$(BUILD)/nut_stubs.o: $(SOYNUT)/firmware/emu41gcc_compat/nut_stubs.c | $(BUILD)
	$(CC) $(STD) $(INC) -c -o $@ $<

$(BUILD)/nut_globals.o: $(SOYNUT)/firmware/emu41gcc_compat/nut_globals.c | $(BUILD)
	$(CC) $(STD) -fcommon $(INC) -c -o $@ $<

$(BUILD)/nut_rom.o: $(SOYNUT)/firmware/emu41gcc_compat/nut_rom.c | $(BUILD)
	$(CC) $(STD) -fcommon $(INC) -c -o $@ $<

$(BUILD)/hp41_key_bridge.o: $(SOYNUT)/firmware/hp41_key_bridge.c | $(BUILD)
	$(CC) $(STD) -fcommon $(INC) -c -o $@ $<

$(BUILD)/hp41_key_hold_bridge.o: $(SOYNUT)/firmware/hp41_key_hold_bridge.c | $(BUILD)
	$(CC) $(STD) -fcommon $(INC) -c -o $@ $<

$(BUILD)/hp41_hpil_controller.o: $(SOYNUT)/firmware/hp41_hpil_controller.c | $(BUILD)
	$(CC) $(STD) -fcommon $(INC) -c -o $@ $<

$(BUILD)/hp41_hpil_video_bridge.o: $(SOYNUT)/firmware/hp41_hpil_video_bridge.c | $(BUILD)
	$(CC) $(STD) -fcommon $(INC) -I$(SOYNUT)/ilvideo-native/core -c -o $@ $<

$(BUILD)/hpil_device.o: $(SOYNUT)/ilvideo-native/core/hpil_device.c | $(BUILD)
	$(CC) $(STD) -I$(SOYNUT)/ilvideo-native/core -c -o $@ $<

$(BUILD)/ilvideo_term.o: $(SOYNUT)/ilvideo-native/core/ilvideo_term.c | $(BUILD)
	$(CC) $(STD) -I$(SOYNUT)/ilvideo-native/core -c -o $@ $<

$(BUILD)/ilvideo_device.o: $(SOYNUT)/ilvideo-native/core/ilvideo_device.c | $(BUILD)
	$(CC) $(STD) -I$(SOYNUT)/ilvideo-native/core -c -o $@ $<

# --- This project's own sources. ---

$(BUILD)/mftest_rom.o: ../build/mftest_rom.c | $(BUILD)
	$(CC) $(STD) $(INC) -c -o $@ $<

$(BUILD)/nut_rom_multifocal.o: nut_rom_multifocal.c | $(BUILD)
	$(CC) $(STD) -fcommon -Wall -Wextra -Wpedantic -Werror $(INC) -c -o $@ $<

$(BUILD)/nut_rom_cx.o: nut_rom_cx.c | $(BUILD)
	$(CC) $(STD) -fcommon -Wall -Wextra -Wpedantic -Werror $(INC) -c -o $@ $<

$(BUILD)/cxfuns_rom.o: ../build/cxfuns_rom.c | $(BUILD)
	$(CC) $(STD) $(INC) -c -o $@ $<

$(BUILD)/xnut_rom.o: ../build/xnut_rom.c | $(BUILD)
	$(CC) $(STD) $(INC) -c -o $@ $<

$(BUILD)/xm_probe_test.o: xm_probe_test.c | $(BUILD)
	$(CC) $(STD) -fcommon -Wall -Wextra -Wpedantic -Werror $(INC) -c -o $@ $<

$(BUILD)/xm_probe_test: $(BUILD)/xm_probe_test.o $(BUILD)/nut_rom_cx.o $(BUILD)/cxfuns_rom.o \
                         $(BUILD)/xnut_rom.o \
                         $(BUILD)/nutcpu.o $(BUILD)/display.o \
                         $(BUILD)/nut_stubs.o $(BUILD)/nut_globals.o \
                         $(BUILD)/hp41_key_bridge.o $(BUILD)/hp41_key_hold_bridge.o \
                         $(BUILD)/hp41_hpil_controller.o $(BUILD)/hp41_hpil_video_bridge.o \
                         $(BUILD)/hpil_device.o $(BUILD)/ilvideo_term.o $(BUILD)/ilvideo_device.o
	$(CC) -o $@ $^

$(BUILD)/desas41_trace.o: $(SOYNUT)/emu41gcc/desas41.c | $(BUILD)
	$(CC) $(STD) -include $(SOYNUT)/tools/desas_proto.h -c -o $@ $<

$(BUILD)/xm_trace_test.o: xm_trace_test.c | $(BUILD)
	$(CC) $(STD) -fcommon -Wall -Wextra -Wpedantic -Werror $(INC) -include $(SOYNUT)/tools/desas_proto.h -c -o $@ $<

$(BUILD)/xm_trace_test: $(BUILD)/xm_trace_test.o $(BUILD)/desas41_trace.o $(BUILD)/nut_rom_cx.o $(BUILD)/cxfuns_rom.o \
                         $(BUILD)/xnut_rom.o \
                         $(BUILD)/nutcpu.o $(BUILD)/display.o \
                         $(BUILD)/nut_stubs.o $(BUILD)/nut_globals.o \
                         $(BUILD)/hp41_key_bridge.o $(BUILD)/hp41_key_hold_bridge.o \
                         $(BUILD)/hp41_hpil_controller.o $(BUILD)/hp41_hpil_video_bridge.o \
                         $(BUILD)/hpil_device.o $(BUILD)/ilvideo_term.o $(BUILD)/ilvideo_device.o
	$(CC) -o $@ $^

$(BUILD)/phase0_loop_test.o: phase0_loop_test.c | $(BUILD)
	$(CC) $(STD) -fcommon -Wall -Wextra -Wpedantic -Werror $(INC) -c -o $@ $<

$(BUILD)/phase0_loop_test: $(BUILD)/phase0_loop_test.o $(BUILD)/nut_rom_multifocal.o \
                            $(BUILD)/nutcpu.o $(BUILD)/display.o $(BUILD)/rom_images.o \
                            $(BUILD)/mftest_rom.o \
                            $(BUILD)/nut_stubs.o $(BUILD)/nut_globals.o $(BUILD)/nut_rom.o \
                            $(BUILD)/hp41_key_bridge.o $(BUILD)/hp41_key_hold_bridge.o \
                            $(BUILD)/hp41_hpil_controller.o $(BUILD)/hp41_hpil_video_bridge.o \
                            $(BUILD)/hpil_device.o $(BUILD)/ilvideo_term.o $(BUILD)/ilvideo_device.o
	$(CC) -o $@ $^

run: all
	./$(BUILD)/phase0_loop_test

clean:
	rm -rf $(BUILD)
