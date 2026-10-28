# Monster Hunter Portable 1 save editor 

 Monster Hunter Portable 1 (MHP1) (ULJM05066) Saveata editor. It supports **decrypted** `MHPSP.BIN` save file. This project does **not** decrypt/encrypt PSP savedata, update `PARAM.SFO`, or handle `SAVEDATA_PARAMS` PSP-level crypto. Back up saves before editing.

In order to decrypt a save, you can do it in PPSSPP by deactivating savedata encryption.

# Usage 

## Build the dependency-free core and CLI

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/mhp1_cli MHPSP.BIN MHPSP_EDITED.BIN --slot 0 --name 'Name' --gender female --face 24 --hair 13 --zenny 7000
```

Windows (Visual Studio): use `cmake --build build --config Release` and find executables under `build/Release/`.

## Build the separate ImGui interface

Requires CMake, a C++17 compiler, Git, and OpenGL development libraries. Configure with network access for Dear ImGui and (if not installed) GLFW:

```sh
cmake -S . -B build-gui -DMHP1_BUILD_GUI=ON
cmake --build build-gui --config Release
./build-gui/mhp1_gui
```

On macOS, install CMake and GLFW (`brew install cmake glfw`) if needed. On Linux, install your distro's OpenGL/X11 development dependencies for GLFW. Windows builds can use Visual Studio and CMake.

The GUI uses a **path text box** instead of an OS file picker, to keep the demo minimal and portable. It does not yet bundle fonts with Japanese glyph coverage; Japanese UTF-8 input is still stored correctly, but glyph rendering may be missing until a suitable system font is configured.

## File format: game-tested offsets (slot-relative)

| Offset | Meaning | Format |
|---|---|---|
| `0x0001` | Gender | `0` male, `1` female |
| `0x0003` | Face | `0..23` |
| `0x0004` | Hairstyle | `0..12` |
| `0x001C` | Name copy 1 | UTF-16LE-compatible |
| `0x0034` | Zenny | LE32 |
| `0x4378` | Name copy 2 | UTF-16LE-compatible |

Slot base: `0x1000 + slot * 0x5000`. Slot 0 is game-tested; other slots follow the observed layout but require independent in-game verification. Character block size `0x46A0`. Total decrypted save size `0x30000`. The checksum is the modulo-2^32 sum of all LE32 words from `0x00000` through `0x2FFF3`, written LE32 at `0x2FFF4`. Preserve bytes `0x2FFF8..0x2FFFF`.

Name writes are **conservatively limited to four UTF-16 code units** in this initial implementation. This is not a claim about the game's actual maximum. Both name copies are updated, and each 8-byte tested area is padded with zeroes. Never use a Python variable-length bytearray slice assignment for save fields: it can shift the rest of the save.

## Architecture

`mhp1_save` is dependency-free and independent of ImGui. `mhp1_cli` and `mhp1_gui` are separate front ends. 

## Known limitations

- Name capacity beyond four UTF-16 code units is not yet proven. Supplementary Unicode code points are rejected for names by the conservative name writer.
- Gender, face, and hairstyle are byte selectors. Face and hairstyle values are verified against the tested MHP1 character creator ranges; 
- Character slot occupancy is not detected. Do not edit empty slots until the slot header/occupancy format is verified.
- GUI zenny input currently uses signed 32-bit integer controls (range `0..2147483647`), while the core supports full uint32.
- No live PSP memory editing, no native recompilation hook, no graphical character preview, and no save encryption/decryption.


## GUI v0.2 additions

- Drag and drop a decrypted `MHPSP.BIN` onto the GLFW window.
- Built-in ImGui file browser (directories and `.BIN` files). No native dialog dependency.
- Slot radio buttons with a **tentative** occupancy heuristic (`character_base + 0x00 == 1`). The heuristic has not been validated across all character slots. Empty/unrecognized slots cannot be exported.
- Back up the original input to `<input>.bak` (created only if absent); back up an existing output to `<output>.bak` before overwriting. Export remains to a separate file.
- Names 1–4 UTF-16 code units are tested. 5–8 are **experimental** and require enabling a checkbox in the GUI. The library/CLI allow 1–8. The field has zero padding through 9 UTF-16 units in observed saves, but the actual game maximum is unverified. Avoid non-BMP/surrogate characters until tested.
- The built-in file browser is not a native OS file dialog. Drag-and-drop is implemented using `glfwSetDropCallback`.
- ImGui's default font may not render Japanese glyphs; loading a CJK font is a future task.

