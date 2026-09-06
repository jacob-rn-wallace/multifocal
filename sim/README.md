# MultiFOCAL interactive simulator (`sim/`)

The real, hand-usable counterpart to `test/`'s automated keystroke-
injection tests: a host-native SDL2 window running the exact same real
Nut CPU core and real ROM images `test/cv82180a_*_test.c` already
verified correct, but driven by an actual keyboard/mouse instead of a
C program feeding bytes. Boots a base HP-41C/CV OS with the real,
user-sourced HP 82180A Extended Functions/Memory module wired at page 5
(Port 2) and MultiFOCAL's own `frames.mod` at page 6 (Port 3) - the
identical CV+82180A configuration this project's own real-hardware
target verification already covers. See `mf_sim_main.c`'s own header
for exactly what's copied from soynut's `sim/sim_main.c` versus added.

## Build and run

Requires SDL2 (`brew install sdl2` on macOS), a sibling `../soynut`
checkout with its own `roms/rom_images.c` already generated (soynut's
own "bring your own ROM" requirement), and this repo's own
`build/frames_rom.c` + `build/e82180a_rom.c` already generated:

```
make mod             # from the repo root - src/frames.s -> build/frames_rom.c
make cv82180a-roms    # from the repo root - the real 82180A.MOD -> build/e82180a_rom.c
make -C sim run       # build and run the interactive simulator
```

## Controls

Same mapping as soynut's own `sim/` (`sim_keyboard.c`, reused
unmodified) - see that project's `sim/README.md` for the full table.
In short: digits/operators/ENTER/backspace map directly; letters are
ALPHA-mode keys; `F9`=XEQ, `` ` ``=ALPHA, `F2`=PRGM, `F3`=SHIFT,
`F1`=ON, `TAB`=USER, `F4`=SST, `F5`=BST, `F6`=X&lt;&gt;Y, `F7`=R&darr;,
`F8`/`SPACE`=R/S.

## Using MultiFOCAL

One-time setup, each cold boot (`CRFLD` creates the frame stack file;
it errors on a repeat call, so don't redo this after continuous memory
has already restored a session with it created - see "Continuous
memory" below):

1. `` ` `` (ALPHA), type `MFSTK`, `` ` `` (ALPHA) again
2. Type `35`
3. `F9` (XEQ), `` ` `` (ALPHA), type `CRFLD`, `` ` `` (ALPHA)
4. `` ` ``, type `MFSTK`, `` ` `` again; type `1`; `F9`, `` ` ``, type
   `SEEKPTA`, `` ` `` (the mandatory post-`CRFLD` seek - see
   `CLAUDE.md`'s compatibility-testing section for why this is needed)

A real FOCAL program can collapse this whole sequence into a single
`XEQ ALPHA "MFINIT" ALPHA` - see `test/mfinit_test.c` and `CLAUDE.md`'s
"MFINIT" section for the actual subroutine and why it can't be built
into MultiFOCAL's own MCODE. **Not usable from this window as-is,
though**: recording `MFINIT` needs `LBL`/`RTN`, and those aren't in
`sim_keyboard.c`'s key mapping either (see the limitation below) - it
would need to be pre-loaded via raw PTY byte injection (the same
technique `test/mfinit_test.c` itself uses) rather than typed in here.

From there, the real functions are all reachable the same way - type a
value, `F9` (XEQ), `` ` `` (ALPHA), type the name, `` ` `` (ALPHA):
`LCLS`, `LCLX`, `LRST` (MultiFOCAL's own), and `SEEKPTA`/`SAVEX`/`GETX`
(the real 82180A primitives local-variable access is built on - see
`CLAUDE.md`'s "LSTO/LRCL retired" section for the calling convention).

**Known limitation, inherited from soynut's own `sim_keyboard.c`**:
`STO`/`RCL`/`GTO`/`LBL`/`RTN` have no key mapping in the SDL window (the
same tooling gap `CLAUDE.md`'s compatibility-testing session found and
worked around with raw keycode injection - see `test/hp41_raw_keys.h`).
That means this simulator is great for interactively exercising
MultiFOCAL's functions live, but can't currently be used to key in a
full multi-line stored FOCAL program (like `test/demo_local_scoping_test.c`'s
own `MFDEMO`/`INNER`) through the window itself.

## Continuous memory

Persists to `sim/soynut_sim_persist.bin` (gitignored, separate from
soynut's own file of the same name in `../soynut/sim/`) - a real
restart restores the last session, `MFSTK` included, rather than a
fresh `MEMORY LOST` boot. Delete that file to force a cold start.
