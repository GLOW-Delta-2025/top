#include <TeensyDMX.h>
#include "../lib/CmdLib.h"
#include "../lib/PingPong.cpp"

using namespace qindesign::teensydmx;

// ---- UNIVERSES ----
Sender dmx1(Serial1);
Sender dmx2(Serial2);

// ---- BAR CONFIG ----
const int SEGMENTS_PER_BAR    = 18;
const int CHANNELS_PER_SEGMENT= 6;
const int CHANNELS_PER_BAR    = SEGMENTS_PER_BAR * CHANNELS_PER_SEGMENT;
const int NUM_BARS            = 5;   // 4 on DMX1 + 1 on DMX2 (plus the beam on DMX2)

// ---- BEAM CONFIG ----
const int BEAM_START_CH      = 109;  // DMX start address (Universe 2)
const int BEAM_PAN_CH        = BEAM_START_CH + 0;   // CH1 - Pan
const int BEAM_PAN_FINE_CH   = BEAM_START_CH + 1;   // CH2 - Pan Fine
const int BEAM_TILT_CH       = BEAM_START_CH + 2;   // CH3 - Tilt
const int BEAM_TILT_FINE_CH  = BEAM_START_CH + 3;   // CH4 - Tilt Fine
const int BEAM_SPEED_CH      = BEAM_START_CH + 4;   // CH5 - Pan/Tilt speed
const int BEAM_DIMMER_CH     = BEAM_START_CH + 5;   // CH6 - Dimmer
const int BEAM_STROBE_CH     = BEAM_START_CH + 6;   // CH7 - Strobe
const int BEAM_COLOUR_CH     = BEAM_START_CH + 7;   // CH8 - Colour wheel
const int BEAM_FOCUS_CH      = BEAM_START_CH + 12;  // CH13 - Focus

// ---- CONSTANTS / TUNING ----
// Colour wheel
const uint8_t YELLOW_INDEX = 0;      // adjust to your fixture’s yellow
const uint8_t WHITE_INDEX  = 0;        // assume 0 is open/white

// Aim constants (flip to 255 if your fixture's "up" is inverted)
const uint8_t BEAM_TILT_UP      = 128; // coarse tilt for straight up
const uint8_t BEAM_TILT_UP_FINE = 128; // fine trim

// Animation timing
const uint32_t FAST_PHASE_MS    = 15000; // 10 seconds of fast shooting stars
const uint32_t FADE_PHASE_MS    = 5000;  // 5 seconds fade out
const uint16_t START_STEP_MS    = 25;    // starting slower speed (higher ms = slower)
const uint16_t END_STEP_MS      = 2;    // ending fast speed (lower ms = faster)
const uint16_t FAST_BURST_MS    = 100;   // short burst for fast phase
const uint16_t FAST_SHOT_INT_MS = 0;     // immediate relaunch after burst

// Visual feel
const uint8_t TIP_BRIGHTNESS    = 255;  // brightness of the moving tip
const uint8_t TRAIL_BRIGHTNESS  = 160;  // base brightness of the trail
const int     TRAIL_LENGTH      = 5;    // number of segments in the trail
const bool    USE_STROBE_BURST  = false;// set true if you want a strobey sky burst

const int WHITE_OFFSET = 4;
// your original "Yellow channel"

// ---- STATE ----
enum ShowState { WAITING, FAST_STARS, FADE_OUT };
ShowState state = WAITING;
uint32_t phaseStartTime = 0;

uint32_t lastStepTime = 0;
int shotPos = -1;              // -1 means “not started yet”; 0..SEGMENTS_PER_BAR-1 = active
bool burstActive = false;
uint32_t burstEndTime = 0;
uint32_t nextShotTime = 0;

String serialBuffer;      // Buffer for incoming serial commands

HardwareSerial* ComSerial = &Serial3;  // Change to prefered Serial port

// Utility: set a channel on the correct universe based on bar index
inline void setBarCh(int bar, int chOffset, uint8_t val) {
  if (bar < 4) {
    dmx1.set(bar * CHANNELS_PER_BAR + chOffset, val);
  } else {
    dmx2.set((bar - 4) * CHANNELS_PER_BAR + chOffset, val);
  }
}

// Clear all channels on all bars
void clearBars() {
  for (int bar = 0; bar < NUM_BARS; ++bar) {
    for (int ch = 1; ch <= CHANNELS_PER_BAR; ++ch) {
      setBarCh(bar, ch, 0);
    }
  }
}

// Set all bars to white at given brightness (using RGB for white)
void setAllWhite(uint8_t brightness) {
  for (int bar = 0; bar < NUM_BARS; ++bar) {
    for (int seg = 0; seg < SEGMENTS_PER_BAR; ++seg) {
      const int base = seg * CHANNELS_PER_SEGMENT;
      setBarCh(bar, base + WHITE_OFFSET, brightness);
    }
  }
}

// Render one upward shot frame across all bars with enhanced trail
void renderShotFrame(int pos) {
  for (int bar = 0; bar < NUM_BARS; ++bar) {
    for (int seg = 0; seg < SEGMENTS_PER_BAR; ++seg) {
      const int whiteCh = seg * CHANNELS_PER_SEGMENT + WHITE_OFFSET;
      uint8_t v = 0;

      if (seg == pos) {
        v = TIP_BRIGHTNESS;  // tip
      } else if (seg < pos && seg >= pos - TRAIL_LENGTH) {
        // Fading trail: brighter closer to tip
        int distance = pos - seg;
        v = TRAIL_BRIGHTNESS * (TRAIL_LENGTH - distance + 1) / (TRAIL_LENGTH + 1);
      } else {
        v = 0;
      }

      setBarCh(bar, whiteCh, v);
    }
  }
}

// Update the moving-head beam
void updateBeam(uint8_t dimmer, uint8_t color_index, bool use_strobe = false) {
  // Aim straight up and set focus/speed
  dmx2.set(BEAM_PAN_CH,       128);
  dmx2.set(BEAM_PAN_FINE_CH,  0);
  dmx2.set(BEAM_TILT_CH,      BEAM_TILT_UP);
  dmx2.set(BEAM_TILT_FINE_CH, BEAM_TILT_UP_FINE);
  dmx2.set(BEAM_SPEED_CH,     20);     // slow/smooth pan/tilt speed
  dmx2.set(BEAM_FOCUS_CH,     0);    // tightened focus
  dmx2.set(BEAM_COLOUR_CH,    color_index);

  dmx2.set(BEAM_DIMMER_CH,    dimmer);
}

void setup() {
  ComSerial->begin(115200, SERIAL_8N1);
  delay(1000);
  dmx1.begin();
  dmx2.begin();
  randomSeed(analogRead(A0));

  PingPong.init(45000, ComSerial);

  clearBars();
  updateBeam(0, YELLOW_INDEX, false);

  // Start in waiting state
  state = WAITING;
  serialBuffer = "";
}

void loop() {
  const uint32_t now = millis();

  while (ComSerial->available()) {
    char c = ComSerial->read();
    serialBuffer += c;
    if (serialBuffer.endsWith("##")) {
      Serial.println(serialBuffer);
      cmdlib::Command parsedCmd;
      String err;

      if (!cmdlib::parse(serialBuffer, parsedCmd, err)) {
        cmdlib::Command errResp;
        errResp.addHeader("MASTER");
        errResp.msgKind = "ERROR";
        errResp.command = parsedCmd.command;
        errResp.setNamed("message", err);
        ComSerial->println(errResp.toString());
      }
      if (parsedCmd.command == "PING") {
        PingPong.processCommand(parsedCmd);
      }

      if (parsedCmd.msgKind != "REQUEST") {
        cmdlib::Command errResp;
        errResp.addHeader("MASTER");
        errResp.msgKind = "ERROR";
        errResp.command = parsedCmd.command;
        errResp.setNamed("message", "Invalid message kind");
        ComSerial->println(errResp.toString());
      }


      if (parsedCmd.command == "START_CLIMAX_TOP") {
        // Send confirm immediately
        cmdlib::Command confirm;
        confirm.addHeader("TOP");
        confirm.addHeader("MASTER");
        confirm.msgKind = "CONFIRM";
        confirm.command = "START_CLIMAX_TOP";
        ComSerial->print(confirm.toString());

        // Start the animation
        state = FAST_STARS;
        phaseStartTime = now;
        nextShotTime = now;
        shotPos = -1;
        burstActive = false;
      }


      serialBuffer = "";
    }
  }
  // Run the show only if not waiting
  if (state == FAST_STARS) {
    // Check if fast phase over
    if (now - phaseStartTime >= FAST_PHASE_MS) {
      state = FADE_OUT;
      phaseStartTime = now;
      clearBars();  // clear any ongoing stars
      shotPos = -1;
      burstActive = false;
      // Fade will handle white in its code
    } else {
      // Compute current step interval based on progress (accelerating: slower to faster)
      float progress = (float)(now - phaseStartTime) / FAST_PHASE_MS;
      uint16_t current_step_ms = START_STEP_MS - (uint16_t)((START_STEP_MS - END_STEP_MS) * progress);

      // Handle burst timing
      if (burstActive && now >= burstEndTime) {
        burstActive = false;
        updateBeam(0, YELLOW_INDEX, false);
        nextShotTime = now + FAST_SHOT_INT_MS;
      }

      // Render current bar state
      if (shotPos >= 0) {
        renderShotFrame(shotPos);
      }

      // Advance the upward shot
      if (shotPos >= 0 && now - lastStepTime >= current_step_ms) {
        lastStepTime += current_step_ms;  // anti-drift, approx since variable

        shotPos++;

        if (shotPos >= SEGMENTS_PER_BAR) {
          shotPos = -1;
          burstActive = true;
          burstEndTime = now + FAST_BURST_MS;
          updateBeam(255, YELLOW_INDEX, USE_STROBE_BURST);  // fire the beam
        }
      }

      // Start a new shot if ready
      if (shotPos == -1 && !burstActive && now >= nextShotTime) {
        shotPos = 0;
        lastStepTime = now;
      }
    }
  } else if (state == FADE_OUT) {
    float progress = (float)(now - phaseStartTime) / FADE_PHASE_MS;
    if (progress >= 1.0f) {
      // End fade, send done, back to waiting
      updateBeam(0, WHITE_INDEX, false);

      // Send done message
      cmdlib::Command doneCmd;
      doneCmd.addHeader("MASTER");
      doneCmd.msgKind = "REQUEST";
      doneCmd.command = "CLIMAX_DONE_TOP";
      ComSerial->print(doneCmd.toString());

      state = WAITING;
    } else {
      uint8_t brightness = (uint8_t)(255 * (1.0f - progress));
      updateBeam(brightness, WHITE_INDEX, false);
    }
  }
}