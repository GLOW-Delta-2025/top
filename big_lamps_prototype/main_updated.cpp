
// Teensy: drive two DMX shields (mirrored output)
// - Serial1 (pin 1 TX)  -> Universe 1 (3 lamps)
// - Serial2 (pin 8 TX)  -> Universe 2 (2 lamps)

#include <TeensyDMX.h>
#include <Arduino.h>
using namespace qindesign::teensydmx;

// Two DMX output universes
Sender dmx1(Serial1);  // Universe 1 (pin 1 TX)
Sender dmx2(Serial2);  // Universe 2 (pin 8 TX)

// We actively drive channels 1..108 on each universe
static const int DMX_ACTIVE_CH = 108;
// Define to send different patterns to U1/U2 for validation
//#define VALIDATE_DIFFERENT_UNIS 1

// Simple time base for animations
elapsedMillis tick;
uint8_t phase = 0;

void setup() {
  Serial.begin(115200);

  dmx1.begin();
  dmx2.begin();

  // Initialize all 512 channels to 0 on both universes
  for (int ch = 1; ch <= 512; ++ch) {
    dmx1.set(ch, 0);
    dmx2.set(ch, 0);
  }

  Serial.println("DMX dual-universe started: U1=Serial1(pin1), U2=Serial2(pin8). Mirroring ch 1..108 across both universes.");
}

static inline uint8_t wrap8(int v) { return (uint8_t)(v & 0xFF); }

// Safe logging helper: uses printf on Teensy, falls back to Serial.print elsewhere
static inline void logFrame(uint32_t frame, uint8_t phase, uint8_t u1, uint8_t u2) {
#if defined(ARDUINO_TEENSY41) || defined(CORE_TEENSY)
  Serial.printf("frame=%lu phase=%u ch1=%u/%u\n", (unsigned long)frame, phase, u1, u2);
#else
  Serial.print("frame="); Serial.print(frame);
  Serial.print(" phase="); Serial.print(phase);
  Serial.print(" ch1="); Serial.print(u1);
  Serial.print("/"); Serial.println(u2);
#endif
}

void loop() {
  // Advance animation phase ~every 20 ms
  if (tick >= 20) {
    tick = 0;
    phase++;
  }

  // Generate one pattern buffer for channels 1..108
  // Example: moving gradient
  for (int ch = 1; ch <= DMX_ACTIVE_CH; ++ch) {
    uint8_t v = wrap8((ch * 2) + phase);
    dmx1.set(ch, v);
#ifdef VALIDATE_DIFFERENT_UNIS
    // Optional: different look on U2 to validate hardware paths
    uint8_t v2 = 255 - wrap8((ch * 3) + (phase << 1));
    dmx2.set(ch, v2);
#else
    // Mirrored output
    dmx2.set(ch, v);
#endif
  }

  // Periodic diagnostics
  static uint32_t frame = 0;
  if ((frame++ % 50) == 0) {
    logFrame(frame, phase, dmx1.get(1), dmx2.get(1));
  }

  // Pace updates; TeensyDMX will stream continuously
  delay(5);
}
