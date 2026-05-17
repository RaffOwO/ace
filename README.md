# ZMK Config for ACE

ACE is the two-card version of the keyboard: a one-device ZMK split that uses Taipo-inspired side-local chord logic.

## Build Targets

- Board: `nice_nano_v2`
- Left shield: `ace_left`
- Right shield: `ace_right`
- Right shield is the central side and is the only Bluetooth keyboard the host pairs with.

## Local Build

After the ZMK local build dependencies are installed, run from this folder:

```powershell
zmk west update
```

Build the left half:

```powershell
& "C:\Users\raffa\AppData\Roaming\uv\tools\zmk\Scripts\python.exe" -m west build -s zmk/app -d build/ace_left -b nice_nano_v2 -- -DSHIELD=ace_left -DZMK_CONFIG="E:/Projects/ACE/zmk-config-ace/config" -DZMK_EXTRA_MODULES="E:/Projects/ACE/zmk-config-ace"
```

Build the right half:

```powershell
& "C:\Users\raffa\AppData\Roaming\uv\tools\zmk\Scripts\python.exe" -m west build -s zmk/app -d build/ace_right -b nice_nano_v2 -- -DSHIELD=ace_right -DZMK_CONFIG="E:/Projects/ACE/zmk-config-ace/config" -DZMK_EXTRA_MODULES="E:/Projects/ACE/zmk-config-ace"
```

Current local blocker: CMake is not installed or not on PATH.

## Split Model

The keymap has 26 global positions:

```text
Left card:  L0..L12  = positions 0..12
Right card: R0..R12  = positions 13..25
```

Combos are still side-local: every chord is defined once for the left card and once for the right card, following Taipo's split model.

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
