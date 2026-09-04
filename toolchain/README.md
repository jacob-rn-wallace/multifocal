# Toolchain — bring your own Calypsi

`calypsi-nut-5.18/` (~150MB, gitignored — third-party binaries, not this
project's code) is the Calypsi Nut/HP-41 MCODE assembler+linker
(`asnut`/`lnnut`/`dbnut`/`modtool`/`nlib`), the actively-maintained
successor to NutStudio (itself the successor to the 1990s DOS-only SDK41).
See `../CLAUDE.md`'s "Phase 0 status" section for how it's used here.

To reinstall it (no sudo — extracts the `.pkg` payload directly, same
approach `~/soynut/toolchain/` already uses for its own ARM toolchain):

```
mkdir -p /tmp/calypsi && cd /tmp/calypsi
gh release download 5.18 --repo hth313/Calypsi-tool-chains \
    -p "calypsi-nut-5.18.pkg" -p "CalypsiNutGuide.pdf" \
    -p "CalypsiNutDebuggerGuide.pdf" -p "ReleaseNotes-Nut.md"
pkgutil --expand-full calypsi-nut-5.18.pkg extracted
cp -R extracted/Payload/usr/local/lib/calypsi-nut-5.18 /path/to/multifocal/toolchain/
```

Then add it to `PATH`:

```
export PATH="$(pwd)/toolchain/calypsi-nut-5.18/bin:$PATH"
```

Source: https://github.com/hth313/Calypsi-tool-chains/releases (pin the
exact release tag — 5.18 is what this project verified against; a newer
release should work but hasn't been checked).
