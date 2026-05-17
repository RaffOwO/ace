# ZMK Config for Tessera

Tessera is the two-card version of TheCardV2: each 13-key card runs the same Taipo-inspired local chord logic as a standalone keyboard.

## Build Targets

- Board: `nice_nano_v2`
- Left shield: `tessera_left`
- Right shield: `tessera_right`
- Both shields build standalone firmware. There is no ZMK central/peripheral split role.

## Local Build

After the ZMK local build dependencies are installed, run from this folder:

```powershell
zmk west update
```

Build the left half:

```powershell
& "C:\Users\raffa\AppData\Roaming\uv\tools\zmk\Scripts\python.exe" -m west build -s zmk/app -d build/tessera_left -b nice_nano_v2 -- -DSHIELD=tessera_left -DZMK_CONFIG="E:/Projects/TheCardv2/zmk-config-thecardv2/config" -DZMK_EXTRA_MODULES="E:/Projects/TheCardv2/zmk-config-thecardv2"
```

Build the right half:

```powershell
& "C:\Users\raffa\AppData\Roaming\uv\tools\zmk\Scripts\python.exe" -m west build -s zmk/app -d build/tessera_right -b nice_nano_v2 -- -DSHIELD=tessera_right -DZMK_CONFIG="E:/Projects/TheCardv2/zmk-config-thecardv2/config" -DZMK_EXTRA_MODULES="E:/Projects/TheCardv2/zmk-config-thecardv2"
```

Current local blocker: CMake is not installed or not on PATH.

## Keymap Model

Each card has 13 local positions:

```text
K0..K12 = positions 0..12
```

Each card has the same local alpha, number, symbol, navigation, mouse, function, system, and media logic.

## Local Half Pin Order

Each half follows the same flipped-PCB switch order:

```text
0  1  2  3
4  5  6  7
8  9 10 11 12
```

PCB nets by local position:

```text
top:  P2,  P3,  P4,  P5
home: P6,  P7,  P8,  P9
mod:  P10, P16, P14, P15, P18
```
