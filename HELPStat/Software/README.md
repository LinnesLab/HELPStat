# HELPStat Firmware (PlatformIO)

Firmware for the HELPStat potentiostat — an **Adafruit ESP32-S3 Feather** driving
an Analog Devices AD5941 for electrochemical impedance spectroscopy (EIS), with
BLE control, SD logging, and on-device Levenberg–Marquardt Randles-cell fitting.

Originally an Arduino IDE sketch; this directory is now a self-contained
PlatformIO project. The sketch has been converted to a standard C++ translation
unit (`AD594x_EIS_Demo.cpp`) for clean PlatformIO compilation.

## Layout

| Path | Purpose |
|------|---------|
| [platformio.ini](platformio.ini) | PlatformIO build configuration |
| [AD594x_EIS_Demo/AD594x_EIS_Demo.cpp](AD594x_EIS_Demo/AD594x_EIS_Demo.cpp) | Main program (`src_dir`) |
| [lib/HELPStatLib/](lib/HELPStatLib/) | AD5941 driver, EIS logic, BLE/SD, LMA fitting |
| `App/` | Companion Android app (separate Gradle project) |

## Prerequisites

- [PlatformIO Core](https://platformio.org/install) (CLI) or the PlatformIO IDE
  extension for VS Code.
- A native terminal. The ESP32 platform installer (pioarduino) **rejects
  MSYS/MinGW/Git-Bash** — on Windows use **PowerShell** or **cmd**, not Git Bash.

## Build / Upload / Monitor

```sh
pio run                 # compile (default env: 4MB/2MB-PSRAM Feather)
pio run -t upload       # flash over USB (auto-resets into the bootloader)
pio device monitor      # serial monitor @ 115200 (native USB CDC)
pio run -t upload -t monitor   # flash then monitor
```

If you have the **8MB no-PSRAM** Feather (Adafruit #5323) instead of the default
4MB/2MB-PSRAM board (#5477), append `-e adafruit_feather_esp32s3_nopsram` to the
`run` commands.

In VS Code, use the PlatformIO toolbar (Build / Upload / Monitor) and pick the
environment from the status bar.

The first build downloads the ESP32 toolchain and the `Eigen-Port` dependency,
so it takes a few minutes; later builds are fast.

## Configuration notes

- **Platform:** ESP32 Arduino core 3.x via the community
  [pioarduino](https://github.com/pioarduino/platform-espressif32) fork. The
  stock PlatformIO `espressif32` platform only ships core 2.x, which this
  firmware predates.
- **Board:** Adafruit ESP32-S3 Feather. Two variants are provided as separate
  environments — `adafruit_feather_esp32s3` (#5477, default) and
  `adafruit_feather_esp32s3_nopsram` (#5323). These board definitions enable
  native-USB Serial (`-DARDUINO_USB_CDC_ON_BOOT=1`), so prints appear on the USB
  serial monitor with no extra wiring. The AD5941/SD pin map lives in
  [lib/HELPStatLib/constants.h](lib/HELPStatLib/constants.h) and already matches
  the Feather (SCK 36 / MOSI 35 / MISO 37). Two pins are deliberately reused:
  GPIO21 (Feather NeoPixel power) for SD chip-select and GPIO7 (STEMMA-QT I2C
  power) for the optional button — both fine since neither peripheral is used.
- **Partition:** the board default (1.4 MB app + OTA + UF2). The firmware is
  ~0.75 MB, so it fits with room to spare and UF2 drag-drop updates still work.
  SD logging is used for data, not SPIFFS.
- **Dependency:** `eigen.h` comes from
  [LinnesLab/Eigen-Port](https://github.com/LinnesLab/Eigen-Port), declared in
  both `platformio.ini` and `lib/HELPStatLib/library.json`.
- **`-Wno-error=return-type`** is set so the original sketch — which the Arduino
  IDE accepted with only a warning — compiles unchanged.
