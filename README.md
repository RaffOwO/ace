# ZMK Config for TheCardV2

This is the ZMK firmware config for TheCardV2.

## Build Target

- Board: `nice_nano_v2`
- Shield: `thecardv2`

## Local Build

After the ZMK local build dependencies are installed, run from this folder:

```powershell
zmk west update
```

Then build with West:

```powershell
& "C:\Users\raffa\AppData\Roaming\uv\tools\zmk\Scripts\python.exe" -m west build -s zmk/app -d build/thecardv2 -b nice_nano_v2 -- -DSHIELD=thecardv2 -DZMK_CONFIG="E:/Projects/TheCardv2/zmk-config-thecardv2/config" -DZMK_EXTRA_MODULES="E:/Projects/TheCardv2/zmk-config-thecardv2"
```

Current local blocker: CMake is not installed or not on PATH.

## Pin Order

The shield overlay follows the current KiCad PCB switch order:

```text
0  1  2  3
4  5  6  7
8  9 10 11 12
```

PCB nets by position:

```text
top:  P2,  P3,  P4,  P5
home: P6,  P7,  P8,  P9
mod:  P10, P16, P14, P15, P18
```

Note: `ergogen.yaml` currently names the final row differently from the checked-in KiCad PCB.
