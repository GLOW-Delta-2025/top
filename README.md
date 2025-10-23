# Echoes of Tomorrow - Toppiece Firmware

This repository contains the embedded firmware for the microcontrollers that operate the toppiece in the **Echoes of Tomorrow** GLOW 2025 project.

## Overview
- Written in C++ for Arduino IDE
- Runs on ESP32 WROVER board
- Controls addressable RGB LED strips (WS2812B) via FastLED library
- Receives serial commands to trigger light sequences
- Sends confirmation responses and status messages
- Two-tier communication: Serial Monitor (debugging) and TX/RX pins (external control)

## Main Firmware: `led_strip_control.ino`

### How It Works

#### Hardware Configuration
- **Board**: ESP32 WROVER
- **LED Library**: FastLED 3.10.3+
- **LED Type**: WS2812B addressable RGB strips
- **Default Configuration**: 2 strips enabled (strip 1 and strip 6)
  - Strip 1 (GPIO 2): Represents the main 5-strip array (charging effect)
  - Strip 6 (GPIO 14): Independent element (intense flow effect)
- **Serial Ports**:
  - **Serial**: USB/UART0 at 115200 baud (Arduino IDE Serial Monitor)
  - **Serial2**: Hardware UART1 at 115200 baud (TX/RX pins: GPIO17/GPIO16)

#### Sequence Flow

**State Machine**: Idle → Active Sequence → Complete → Idle

**Active Sequence (60 seconds total)**:

1. **Phase 1 - Charging (30 seconds)**
   - Strip 1 progressively fills with LEDs
   - Color gradient: Blue → White (simulating power buildup)
   - Update rate: Every 100ms
   - Effect: Represents energy accumulation

2. **Phase 2 - Intense Flow (30 seconds)**
   - Strip 6 activates with a flowing stream effect
   - 15-LED stream moves continuously around the strip
   - Gradient: Bright white (head) → Dim blue (tail)
   - Random white sparks (30% chance per frame)
   - Update rate: Every 50ms (faster for intensity)
   - Effect: Represents energy release/climax

#### Serial Command Protocol

**All commands end with `##` delimiter**

##### Incoming Commands (ESP Receives)

| Command | Format | Response |
|---------|--------|----------|
| Start Sequence | `!!TOP:REQUEST:START_CLIMAX_TOP{TIME}##` | `!MASTER:CONFIRM:START_CLIMAX_TOP##` |
| Stop Sequence | `!!TOP:REQUEST:STOP_CLIMAX_TOP##` | `!!MASTER:CONFIRM:STOP_CLIMAX_TOP##` |

**Example**:
```
Send: !!TOP:REQUEST:START_CLIMAX_TOP{1000}##
Receive: !MASTER:CONFIRM:START_CLIMAX_TOP##
[Sequence runs for 60 seconds]
Receive: !!MASTER:CONFIRM:STOP_CLIMAX_TOP##
```

##### Outgoing Confirmations (ESP Sends to Both Serial & Serial2)

- **On START received**: `!MASTER:CONFIRM:START_CLIMAX_TOP##`
- **On completion or manual STOP**: `!!MASTER:CONFIRM:STOP_CLIMAX_TOP##`

---

## Setup Instructions

### Prerequisites
- Arduino IDE 1.8.13 or later
- FastLED library 3.10.3+ (Install via Sketch → Include Library → Manage Libraries)
- ESP32 board package (Install via Tools → Board Manager, search "esp32")

### Installation Steps

1. **Install Libraries**:
   - Open Arduino IDE
   - Go to **Sketch → Include Library → Manage Libraries**
   - Search for "FastLED" and install the latest version
   - Search for "esp32" and install the latest ESP32 board package

2. **Open the Sketch**:
   - Open `led_strip_control.ino` in Arduino IDE

3. **Configure Board**:
   - **Tools → Board**: Select `esp32:esp32:esp32wrover`
   - **Tools → Upload Speed**: 921600
   - **Tools → Flash Freq**: 80 MHz
   - **Tools → Partition Scheme**: Default

4. **Connect Hardware**:
   - USB cable to the ESP32 WROVER
   - LED strip(s) data pin to GPIO 2 (strip 1) and/or GPIO 14 (strip 6)
   - LED strip ground to ESP32 ground
   - LED strip power to external 5V supply

5. **Upload**:
   - Select the COM port (**Tools → Port**)
   - Click **Upload**
   - Serial output will appear at 115200 baud

---

## Debugging Guide

### Method 1: Serial Monitor (USB Debug)

1. **Open Serial Monitor**:
   - Tools → Serial Monitor (or Ctrl+Shift+M)
   - Set baud rate to **115200**

2. **Send Commands**:
   - Type into the input field at the bottom and press **Send**
   - Example: `!!TOP:REQUEST:START_CLIMAX_TOP{0}##`

3. **Monitor Output**:
   - Watch for initialization messages during boot
   - Observe state transitions and timing updates
   - Check for responses and confirmations

4. **Typical Boot Output**:
   ```
   Serial2 initialized on RX=16, TX=17
   Strip 1 initialized on pin 2
   Strip 6 initialized on pin 14
   LED Strip Installation Started
   ```

5. **Sequence Output Example**:
   ```
   Received start climax command with time: 0
   !MASTER:CONFIRM:START_CLIMAX_TOP##
   Starting climax sequence
   Starting strip 1 charging
   Starting strip 6 intense flow
   Climax sequence completed
   !!MASTER:CONFIRM:STOP_CLIMAX_TOP##
   ```

### Method 2: TX/RX External Control (Hardware Debug)

1. **Wire External Device**:
   - Connect external device RX to ESP32 GPIO 17 (TX2)
   - Connect external device TX to ESP32 GPIO 16 (RX2)
   - Share ground

2. **Send Commands**:
   - Send via the external serial device at 115200 baud
   - Monitor responses on the same TX/RX line

3. **Responses Appear On**:
   - Both Serial Monitor (USB)
   - TX/RX pins (external device)

### Troubleshooting

#### LEDs Don't Light Up
- [ ] Check GPIO pins match your wiring (default: 2 and 14)
- [ ] Verify external 5V power connected to strips
- [ ] Confirm ground is shared between ESP32 and LED power supply
- [ ] Check that `#define ENABLE_STRIP1` and `#define ENABLE_STRIP6` are not commented out
- [ ] Verify LED strip is WS2812B (addressable RGB) not simple single-color strip

#### No Serial Output
- [ ] Confirm baud rate is 115200
- [ ] Check USB cable is connected and drivers installed
- [ ] Look at COM port number in Device Manager (Windows) or /dev (Mac/Linux)
- [ ] Try a different USB port

#### Commands Not Recognized
- [ ] Ensure commands end with `##` (two hashes)
- [ ] Check command format matches exactly (case-sensitive)
- [ ] Verify "Line ending" is set to "Newline" in Serial Monitor
- [ ] Monitor output should show parsing in checkSerialCommands()

#### Sequence Timing Off
- [ ] Edit `flashDuration` (default 30000 ms for charging)
- [ ] Edit `burstDuration` (default 30000 ms for flow)
- [ ] Recompile and upload after changes

### Configurable Parameters

Edit these values in `led_strip_control.ino` to customize:

```cpp
// Line 5: LED count per strip (adjust if strips are different length)
#define NUM_LEDS_PER_STRIP 60

// Line 6: Global brightness (0-255)
#define BRIGHTNESS 64

// Line 15-16: Enable/disable strips (comment out to disable)
#define ENABLE_STRIP1
#define ENABLE_STRIP6

// Line 42-43: Duration of each phase (milliseconds)
unsigned long flashDuration = 30000;    // Charging phase
unsigned long burstDuration = 30000;    // Flow phase

// Line 210, 233: Update rates
if (millis() - lastUpdate > 100)   // Charging update: 100ms
if (millis() - lastUpdate > 50)    // Flow update: 50ms
```

---

## File Structure

```
top/
├── led_strip_control.ino       # Main firmware
├── led_strip_plan.md           # Detailed hardware/software plan
├── README.md                   # This file
├── development/
│   ├── example.ino             # Original blink example
│   └── single_strip_demo.ino   # Single strip test sketch
└── .git/                       # Version control
```

---

## Development Hints

### Testing Individual Strips
- Use `development/single_strip_demo.ino` to test one strip independently
- Modify GPIO and LED count, upload, observe behavior

### Gradual Strip Connection
- Comment out unwanted `#define ENABLE_STRIPx` lines
- Code will skip initialization and control of disabled strips
- Allows testing with partial hardware

### Adding More Strips
- Define new pin and CRGB array in the global section
- Add `#define ENABLE_STRIPn` flag
- Add FastLED initialization in `setup()`
- Include control logic in `chargingEffect()` or `intenseFlow()`

---

## References

- **FastLED Documentation**: https://fastled.io/
- **ESP32 Arduino Core**: https://github.com/espressif/arduino-esp32
- **WS2812B Datasheet**: Search online for timing requirements
- **Project Planning**: See `led_strip_plan.md`

---

## Branches
- `main`: Production-ready code
- `develop`: Active development
- `feature/<name>`, `bugfix/<name>`, `hotfix/<name>`: Use Git Flow

## Commit Convention
```text
<type>: <short description>
```
Example: `feat: add LED strip control with serial commands`
