GLOW 2025 — Big Lamps Prototype (Teensy + Dual DMX Shields)

Overview
- Teensy 4.1 drives two TinkerKit T040060 DMX shields as RS‑485 transmitters.
- Two DMX universes: Serial1 (pin 1 TX) for 3 lamps, Serial2 (pin 8 TX) for 2 lamps.
- All 5 lamps behave the same: channels 1–108 are mirrored from U1 to U2.
- Primary code: `top/big_lamps_prototype/main_updated.cpp` (paste into a `.ino` for Arduino IDE if needed). The older `main.cpp` can be ignored.

Hardware setup
- Shields act only as RS‑485 line drivers; DMX is controller‑transmit only.
- Wiring (Teensy 4.1):
	- Shield #1 DI ← Teensy pin 1 (Serial1 TX) → Universe 1 (3 fixtures in chain)
	- Shield #2 DI ← Teensy pin 8 (Serial2 TX) → Universe 2 (2 fixtures in chain)
	- Optional RX lines are not used.
	- Common GND between Teensy, shields, and lamp PSU.
	- DMX/XLR pins: 1=GND, 2=Data− (B), 3=Data+ (A).
	- Termination: last fixture in each chain ON (120 Ω).
	- DE/RE on shields: tie for transmit enable per shield docs (controller is TX‑only).

DMX addressing (assumes 108 channels/fixture)
- Universe 1 chain: A1=1, A2=109, A3=217.
- Universe 2 chain: A4=1, A5=109.
- All fixtures use the same 108‑channel personality. Set via menu/DIP.

Code architecture (main_updated.cpp)
- Libraries: TeensyDMX, Arduino.
- Two Senders: `dmx1(Serial1)` and `dmx2(Serial2)`.
- Drive channels 1..108 each frame from a generated pattern (moving gradient).
- Mirroring: by default the same value `v` is written to U1 and U2 for each channel.
- Validation mode (optional): define `VALIDATE_DIFFERENT_UNIS` to drive U2 with a different pattern for hardware path checks.
- Diagnostics: periodic `logFrame(frame, phase, U1_ch1, U2_ch1)` via Serial.
- Timing: simple `elapsedMillis` tick with `delay(5)` pacing (logic‑level simulation of ~100–200 Hz update cadence).

Build and upload (Arduino IDE)
1) Open Arduino IDE (with Teensyduino installed).
2) Create a new sketch and paste `main_updated.cpp` content (Arduino requires `.ino`).
3) Tools → Board: Teensy 4.1; USB Type: Serial; CPU Speed: 600 MHz.
4) Upload and open Serial Monitor at 115200 baud.

Expected behavior on hardware
- Both universes output identical data on channels 1–108 (mirrored), yielding identical looks on the 3‑lamp chain and the 2‑lamp chain.
- Serial prints every ~50 frames: `frame=... phase=... ch1=U1/U2`.
- If `VALIDATE_DIFFERENT_UNIS` is defined: U2 intentionally differs (inverse/faster modulation) for visual validation.

No‑hardware host simulator
- Location: `top/big_lamps_prototype/host_sim`.
- What it does: stubs Arduino and TeensyDMX APIs and runs `main_updated.cpp` on macOS. This validates logic and logging without real hardware.
- Build and run (macOS):
```zsh
cd ~/Documents/GLOW_2025/top/big_lamps_prototype/host_sim
clang++ -std=c++17 -Iinclude run.cpp -o sim
./sim | head -50
```
- Expected output: the same startup and periodic frame logs you’d see on Teensy, then “Host sim finished.”
- Notes: this does not simulate DMX timing or the physical bus; it exercises the code flow only.

Planned command protocol (USB Serial and later a spare UART)
- During development: send commands over USB Serial; for production also accept via a spare UART (e.g., Serial3).
- Minimal commands (to be added):
	- `FILL <v>` — set ch 1–108 to value 0..255
	- `BLACKOUT` — set all to 0
	- `GRAD <speed>` — gradient animation with speed 1..10
	- `SET <ch> <v>` — set channel
	- `PING` — respond `PONG`

Troubleshooting
- No light: verify fixture start address=1 and correct personality; confirm termination; check DE/RE enable; swap XLR pins 2/3 if needed.
- One universe dark: check correct Teensy pin (1 vs 8), shield power/GND, and cable.
- Serial.printf issues: the code uses a safe `logFrame` helper that falls back to Serial.print when printf isn’t available.
- Editor include squiggles: harmless for Arduino builds; use Arduino IDE or PlatformIO to compile/upload.

Optional: PlatformIO
If you prefer PlatformIO, create a `platformio.ini` (Teensy 4.1, Arduino framework, TeensyDMX lib), then `pio run -t upload` and `pio device monitor -b 115200`.

License
- See repository root for license information (if applicable).

