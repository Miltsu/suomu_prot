# Suomu — Handheld Fish Finder
Metropolia UAS (Milla Juote, Phoebe Kuusijärvi, Matleena Vaara - 2026)

Our team designed a prototype for a handheld fish-finder built for our embedded IoT systems engineering course. 
A handheld unit with a e-paper screen and buttons connects via an extendable pole to a waterproof sensor probe, 
measuring water depth and temperature and sending readings to a companion phone app over Bluetooth.

## Status
In active development, team presentation-ready as of the current sprint. See [Roadmap](#roadmap) below for what's done vs. in progress.




## Hardware

| Component | Part |
|---|---|
| MCU | ESP32-C3-DevKitC-02 (ESP32-C3-WROOM-02 module) |
| Distance sensor | JSN-SR04T waterproof ultrasonic sensor |
| Temperature sensor | NTC 10k thermistor |
| Display | Waveshare 1.54" e-Paper Module, rev 2.1 (200×200, SPI) |
| Op-amps | 2× MCP6022-E/P dual op-amp |
| Battery | 18650 Li-ion, 2600mAh |
| Charging | MicroLipo 3.7V/4.2V charger |
| Power | 0.9–5V → 5V DC-DC step-up (boost) converter |
| Buttons | 2× push buttons |
| GPS | TBD (provided by course, model not yet confirmed) |
| Connectivity | Bluetooth Low Energy (built into ESP32-C3) |

## Pinout

| Function | GPIO |
|---|---|
| NTC thermistor (ADC) | GPIO0 |
| JSN-SR04T TRIG | GPIO2 |
| JSN-SR04T ECHO (via 1kΩ/2kΩ divider) | GPIO3 |
| Display DC | GPIO4 |
| Display RST | GPIO5 |
| Display CLK (SPI) | GPIO6 |
| Display DIN/MOSI (SPI) | GPIO7 |
| Onboard RGB LED | GPIO8 |
| Display CS (SPI) | GPIO10 |
| Display BUSY | GPIO18 |

GPIO19–21 reserved for buttons / GPS UART once finalised.

**Note:** the JSN-SR04T's ECHO line outputs 5V; a 1kΩ/2kΩ resistor divider steps this down to a safe 3.3V for the ESP32-C3's GPIO.

## Software architecture

Built with **ESP-IDF** (not Arduino), developed in **CLion**. Code is split into per-module `.c`/`.h` pairs so the team can work in parallel:

```
main/
├── hello_world_main.c   — app entry point, orchestrates modules
├── jsn_sr04t.c / .h      — ultrasonic distance sensor
├── ntc_thermistor.c / .h — temperature sensor (ADC + Beta-equation conversion)
├── display.cpp / .h      — e-paper display (C++ driver behind a C-compatible interface)
├── ble.c / .h            — BLE service exposing sensor readings to the phone app
components/
└── epaper/                — third-party e-paper driver library (C++), adapted for this ESP-IDF version
```

Most code is C, matching ESP-IDF's own core APIs. The e-paper display uses a third-party C++ driver library; rather than rewriting it, we wrapped its two entry points (`display_init`, `display_show_text`) in `extern "C"` so the rest of our C codebase can call into it directly.



## Building

Requires [ESP-IDF](https://github.com/espressif/esp-idf) (developed against a recent 6.x release) targeting `esp32c3`.

```bash
source <path-to-esp-idf>/export.sh
idf.py set-target esp32c3
idf.py build
idf.py -p <serial-port> flash monitor
```

CLion users: launch CLion from a terminal with the ESP-IDF environment sourced, then set `-DIDF_TARGET=esp32c3` as a CMake option when opening the project.




## Companion app

The phone-side app is built with **MIT App Inventor** and connects to the device over BLE to receive live distance and temperature readings.



## Roadmap

- [x] MCU + toolchain set up (ESP-IDF, CLion, cross-platform team setup)
- [x] Ultrasonic distance sensor — wired, tested, working
- [x] Temperature sensor — wired, tested, working, calibrated against known temperatures
- [x] Onboard LED — working
- [x] E-paper display — driver integrated, text rendering confirmed
- [x] Code split into per-module architecture
- [x] BLE — service/characteristics drafted, hardware testing in progress
- [ ] GPS — blocked on hardware arrival
- [ ] Buttons — pin-budget-constrained; considering an ADC-based button ladder to save GPIOs
- [ ] Power subsystem (battery + charger + boost converter) — parts in hand, not yet assembled end-to-end
- [ ] Final PCB layout (KiCad) — pending full breadboard integration
- [ ] 3D-printed enclosure
- [ ] Final assembly and soldering


## Team

Course project for embedded IoT systems engineering at Metropolia.
