# RP2040 PS3 Controller Fixer

Firmware for an RP2040 board (e.g. RP2040 Zero) that emulates a DualShock 3 controller over USB. Fixes the black screen problem that occurs when mounting an ISO without a physical controller connected — for example when using only the Playstation BD remote or the virtual pad.

## Firmware Variants

Two UF2 files are produced by the build:

- **CFW** — Basic controller emulation only.
- **PS3HEN** — Controller emulation plus an automatic startup combo that activates HEN after a 30-second delay.

Run `build.bat` to build both and place them in the `output` folder.

## Features

### Manual Combo (Boot Button)
Pressing the boot button sends **SELECT + L3 + L1 + R1**, which triggers **Refresh XML + Reload XMB** on the PS3.

### Automatic Startup Combo (PS3HEN variant only)
30 seconds after power-on, the firmware automatically sends LEFT + LEFT + X to auto click on Enable HEN.

## LED Indicators

| Color | Meaning |
|-------|---------|
| 🔴 Red | Waiting for USB connection |
| 🟢 Green | USB connected (shown for 3 seconds) |
| 🔵 Blue | Combo being sent |
| ⚫ Off | Idle / connected |