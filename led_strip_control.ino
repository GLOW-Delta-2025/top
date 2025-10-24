/*
  GLOW 2025 - Echoes of Tomorrow Toppiece LED Control

  This firmware controls 6 addressable LED strips (WS2812B) for the toppiece light installation.
  The system uses a dual-communication architecture:
  - Serial (USB): Debug messages and commands
  - Serial2 (TX/RX pins): External control and responses

  CURRENT CONFIGURATION:
  - Only strips 1 and 6 are enabled for testing
  - Strip 1: Represents the 5-strip main array (charging effect)
  - Strip 6: Independent climax element (intense flow effect)

  ENABLING ALL 6 STRIPS FOR PRODUCTION:
  1. Uncomment all #define ENABLE_STRIPx lines (lines 20-25)
  2. Wire each strip to its assigned GPIO pin
  3. Supply external 5V power to each strip (separate from ESP32)
  4. Share ground between ESP32 and LED power supply
  5. Recompile and upload

  SYNCHRONIZED CONTROL:
  The first 5 strips (1-5) use the same chargingEffect() function.
  They all:
  - Start charging simultaneously when sequence begins
  - Use identical blue-to-white gradient fill pattern
  - Complete charging at the same time
  
  Strip 6 is independent and uses intenseFlow() for the climax phase.

  ARCHITECTURE:
  - chargingEffect(): Fills strips 1-5 progressively (0% to 100% over 30s)
  - intenseFlow(): Animated stream on strip 6 with random sparks (30s)
  - checkSerialCommands(): Parses commands from both Serial and Serial2
  - Command protocol: !!TOP:REQUEST:START_CLIMAX_TOP{TIME}## / !!TOP:REQUEST:STOP_CLIMAX_TOP##
  - Responses: !MASTER:CONFIRM:START_CLIMAX_TOP## / !!MASTER:CONFIRM:STOP_CLIMAX_TOP##
*/

#include <FastLED.h>

// LED Strip Configuration
#define NUM_LEDS_PER_STRIP 60  // Adjust based on your strip length
#define BRIGHTNESS 64

// Pin assignments for each strip
// To enable all 6 strips, uncomment the respective #define ENABLE_STRIPx lines below
#define STRIP1_PIN 2    // Strip 1 (part of main 5-array)
#define STRIP2_PIN 4    // Strip 2 (part of main 5-array)
#define STRIP3_PIN 5    // Strip 3 (part of main 5-array)
#define STRIP4_PIN 12   // Strip 4 (part of main 5-array)
#define STRIP5_PIN 13   // Strip 5 (part of main 5-array)
#define STRIP6_PIN 14   // Strip 6 (independent climax element)

// Enable/disable strips (comment out to disable)
// For PRODUCTION with all 6 strips, uncomment all lines below:

// TESTING CONFIGURATION (currently active):
// Only strips 1 and 6 enabled. Strips 1-5 will be synchronized when all are enabled.
#define ENABLE_STRIP1  // Main 5-strip array (charging effect)
#define ENABLE_STRIP2
//#define ENABLE_STRIP3
//#define ENABLE_STRIP4
//#define ENABLE_STRIP5
#define ENABLE_STRIP6  // Independent climax element (intense flow)

// Serial command
// External TX/RX communication allows the toppiece to receive commands from a host controller
// and send back status confirmations on the same physical TX/RX header.
#define UART_RX_PIN 16  // RX pin for external commands (Serial2)
#define UART_TX_PIN 17  // TX pin for external responses (Serial2)

// LED array declarations: only enabled strips are instantiated
// This allows gradual hardware connection without code changes
#ifdef ENABLE_STRIP1
CRGB strip1[NUM_LEDS_PER_STRIP];
#endif
#ifdef ENABLE_STRIP2
CRGB strip2[NUM_LEDS_PER_STRIP];
#endif
#ifdef ENABLE_STRIP3
CRGB strip3[NUM_LEDS_PER_STRIP];
#endif
#ifdef ENABLE_STRIP4
CRGB strip4[NUM_LEDS_PER_STRIP];
#endif
#ifdef ENABLE_STRIP5
CRGB strip5[NUM_LEDS_PER_STRIP];
#endif
#ifdef ENABLE_STRIP6
CRGB strip6[NUM_LEDS_PER_STRIP];
#endif

// Lookup table of strips participating in the charging phase (strips 1-5).
// Each pointer appears only if the corresponding strip is enabled, allowing
// chargingEffect() to iterate across all active strips with identical logic.
static CRGB* const chargingStrips[] = {
#ifdef ENABLE_STRIP1
  strip1,
#endif
#ifdef ENABLE_STRIP2
  strip2,
#endif
#ifdef ENABLE_STRIP3
  strip3,
#endif
#ifdef ENABLE_STRIP4
  strip4,
#endif
#ifdef ENABLE_STRIP5
  strip5,
#endif
};

static const size_t chargingStripCount = sizeof(chargingStrips) / sizeof(chargingStrips[0]);

// State machine management
enum DeviceState { IDLE, RUNNING };
DeviceState currentState = IDLE;

// Timing variables for the climax sequence
// Sequence phases:
// 1. Charging phase (flashDuration): Strips 1-5 progressively fill with blue→white gradient
// 2. Flow phase (burstDuration): Strip 6 activates with intense animated stream
unsigned long startTime;
unsigned long flashDuration = 30000;  // 30 seconds charging for strips 1-5
unsigned long burstDuration = 30000;  // 30 seconds for intense flow on strip 6
bool strips1to5Filled = false;
bool strip6Active = false;
bool strip1Flashing = false;
bool strip6Bursting = false;

// Serial command buffer
String serialBuffer = "";
String serial2Buffer = "";  // buffer for Serial2 (TX/RX pins)

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  // Initialize Serial2 for external TX/RX communication
  // This allows a host controller (master) to send commands and receive responses
  // on the physical TX/RX header pins, independent of USB Serial.
  Serial2.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  Serial.println("Serial2 initialized on RX=" + String(UART_RX_PIN) + ", TX=" + String(UART_TX_PIN));

  // Initialize FastLED for each enabled strip
  // When a strip's #define ENABLE_STRIPx is active, FastLED manages that GPIO and LED array.
  // When disabled (commented), the code is skipped entirely, avoiding errors on unconnected pins.
#ifdef ENABLE_STRIP1
  FastLED.addLeds<WS2812B, STRIP1_PIN, GRB>(strip1, NUM_LEDS_PER_STRIP);
  Serial.println("Strip 1 initialized on pin " + String(STRIP1_PIN));
#endif
#ifdef ENABLE_STRIP2
  FastLED.addLeds<WS2812B, STRIP2_PIN, GRB>(strip2, NUM_LEDS_PER_STRIP);
  Serial.println("Strip 2 initialized on pin " + String(STRIP2_PIN));
#endif
#ifdef ENABLE_STRIP3
  FastLED.addLeds<WS2812B, STRIP3_PIN, GRB>(strip3, NUM_LEDS_PER_STRIP);
  Serial.println("Strip 3 initialized on pin " + String(STRIP3_PIN));
#endif
#ifdef ENABLE_STRIP4
  FastLED.addLeds<WS2812B, STRIP4_PIN, GRB>(strip4, NUM_LEDS_PER_STRIP);
  Serial.println("Strip 4 initialized on pin " + String(STRIP4_PIN));
#endif
#ifdef ENABLE_STRIP5
  FastLED.addLeds<WS2812B, STRIP5_PIN, GRB>(strip5, NUM_LEDS_PER_STRIP);
  Serial.println("Strip 5 initialized on pin " + String(STRIP5_PIN));
#endif
#ifdef ENABLE_STRIP6
  FastLED.addLeds<WS2812B, STRIP6_PIN, GRB>(strip6, NUM_LEDS_PER_STRIP);
  Serial.println("Strip 6 initialized on pin " + String(STRIP6_PIN));
#endif

  FastLED.setBrightness(BRIGHTNESS);

  // Start timing
  startTime = millis();

  Serial.println("LED Strip Installation Started");
}

// Helper: turn off all enabled strips
static void turnOffAllStrips() {
#ifdef ENABLE_STRIP1
  fill_solid(strip1, NUM_LEDS_PER_STRIP, CRGB::Black);
#endif
#ifdef ENABLE_STRIP2
  fill_solid(strip2, NUM_LEDS_PER_STRIP, CRGB::Black);
#endif
#ifdef ENABLE_STRIP3
  fill_solid(strip3, NUM_LEDS_PER_STRIP, CRGB::Black);
#endif
#ifdef ENABLE_STRIP4
  fill_solid(strip4, NUM_LEDS_PER_STRIP, CRGB::Black);
#endif
#ifdef ENABLE_STRIP5
  fill_solid(strip5, NUM_LEDS_PER_STRIP, CRGB::Black);
#endif
#ifdef ENABLE_STRIP6
  fill_solid(strip6, NUM_LEDS_PER_STRIP, CRGB::Black);
#endif
}

void loop() {
  // Check for serial commands from both USB (Serial) and external TX/RX (Serial2)
  checkSerialCommands();

  unsigned long currentTime = millis();

  // Main state machine: handles RUNNING and IDLE states
  if (currentState == RUNNING) {
    unsigned long elapsedTime = currentTime - startTime;

    // PHASE 1: CHARGING (0-30 seconds)
    // Strips 1-5: Progressive fill from blue to white
    if (!strip1Flashing && elapsedTime < flashDuration) {
      strip1Flashing = true;
      Serial.println("Starting strip 1 charging");
    }

    if (strip1Flashing && elapsedTime < flashDuration) {
      chargingEffect();
    }

    // PHASE 2: INTENSE FLOW (30-60 seconds)
    // Transition point: switch from charging to climax
    if (elapsedTime >= flashDuration && !strip6Bursting) {
      strip6Bursting = true;
      strip1Flashing = false;
      Serial.println("Starting strip 6 intense flow");
    }

    // Strip 6: Animated stream with energy sparks (independent climax element)
    if (strip6Bursting && elapsedTime < flashDuration + burstDuration) {
      intenseFlow();
    }

    // End sequence: transition to IDLE, turn off strips, and signal completion
    if (elapsedTime >= flashDuration + burstDuration) {
      currentState = IDLE;
      strip1Flashing = false;
      strip6Bursting = false;
      // Turn off all strips
#ifdef ENABLE_STRIP1
      fill_solid(strip1, NUM_LEDS_PER_STRIP, CRGB::Black);
#endif
#ifdef ENABLE_STRIP2
      fill_solid(strip2, NUM_LEDS_PER_STRIP, CRGB::Black);
#endif
#ifdef ENABLE_STRIP3
      fill_solid(strip3, NUM_LEDS_PER_STRIP, CRGB::Black);
#endif
#ifdef ENABLE_STRIP4
      fill_solid(strip4, NUM_LEDS_PER_STRIP, CRGB::Black);
#endif
#ifdef ENABLE_STRIP5
      fill_solid(strip5, NUM_LEDS_PER_STRIP, CRGB::Black);
#endif
#ifdef ENABLE_STRIP6
      fill_solid(strip6, NUM_LEDS_PER_STRIP, CRGB::Black);
#endif
      FastLED.show();
      Serial.println("Climax sequence completed. Returning to IDLE.");
      // Send completion confirmation to both Serial and Serial2
      Serial.println("!!MASTER:CONFIRM:STOP_CLIMAX_TOP##");
      Serial2.println("!!MASTER:CONFIRM:STOP_CLIMAX_TOP##");
    }
  } else {
    // IDLE state: Keep strips off and wait for a command.
    turnOffAllStrips();
    // Periodic status ping to show we're alive and ready
    static unsigned long lastPing = 0;
    if (currentTime - lastPing > 10000) { // Ping every 10 seconds
      lastPing = currentTime;
      Serial.println("!!TOP:STATUS:IDLE##");
      Serial2.println("!!TOP:STATUS:IDLE##");
    }
  }

  FastLED.show();
  delay(50);
}

// Function to check for serial commands
void checkSerialCommands() {
  // Local lambda to process complete commands within a buffer
  auto processBuffer = [&](String &buf, const char* sourceTag) {
    int sepIdx;
    while ((sepIdx = buf.indexOf("##")) != -1) {
      String cmd = buf.substring(0, sepIdx + 2);
      buf.remove(0, sepIdx + 2);
      // Trim any CR/LF that might immediately follow
      while (buf.length() > 0 && (buf[0] == '\\r' || buf[0] == '\\n' || buf[0] == ' ')) {
        buf.remove(0, 1);
      }

      cmd.trim();

      if (cmd.startsWith("!!TOP:REQUEST:START_CLIMAX_TOP")) {
        // Extract time if present
        int startIdx = cmd.indexOf('{');
        int endIdx = cmd.indexOf('}');
        if (startIdx != -1 && endIdx != -1 && endIdx > startIdx) {
          String timeStr = cmd.substring(startIdx + 1, endIdx);
          Serial.println(String("(") + sourceTag + ") Start with time: " + timeStr);
        }

        // Confirm and (re)start
        Serial.println("!MASTER:CONFIRM:START_CLIMAX_TOP##");
        Serial2.println("!MASTER:CONFIRM:START_CLIMAX_TOP##");

        currentState = RUNNING;
        startTime = millis();
        strip1Flashing = false; // Reset phase flags
        strip6Bursting = false;
        Serial.println(String("(") + sourceTag + ") Starting (or restarting) climax sequence");
      } else if (cmd.startsWith("!!TOP:REQUEST:STOP_CLIMAX_TOP")) {
        // Stop and go idle
        if (currentState == RUNNING) {
          currentState = IDLE;
          strip1Flashing = false;
          strip6Bursting = false;
          turnOffAllStrips();
          FastLED.show();
          Serial.println(String("(") + sourceTag + ") Stopped sequence. Returning to IDLE.");
          Serial.println("!!MASTER:CONFIRM:STOP_CLIMAX_TOP##");
          Serial2.println("!!MASTER:CONFIRM:STOP_CLIMAX_TOP##");
        }
      } else {
        // Unknown command: ignore silently to avoid clogging
      }
    }
  };

  // Read from USB Serial
  while (Serial.available()) {
    char c = Serial.read();
    serialBuffer += c;
  }
  processBuffer(serialBuffer, "USB");

  // Read from Serial2 (external TX/RX)
  while (Serial2.available()) {
    char c2 = Serial2.read();
    serial2Buffer += c2;
  }
  processBuffer(serial2Buffer, "RX2");
}

// Function for charging effect on strips 1-5
// This function is called during the first 30 seconds of the climax sequence.
// When all 6 strips are enabled, this effect runs simultaneously on strips 1-5,
// creating a synchronized "power accumulation" visual effect.
//
// Effect:
// - Progressively fills the strip from position 0 to the end
// - Color gradient: Blue (start, 0%) → White (end, 100%)
// - All enabled strips receive identical fill progress
// - Update rate: 100ms (smooth but not too fast)
//
// If you want to modify this effect for all 5 strips at once,
// simply edit the CRGB color mapping here.
void chargingEffect() {
  static unsigned long lastUpdate = 0;

  // Faster cadence for a snappier look
  if (millis() - lastUpdate <= 60) {
    return;  // ~16 FPS
  }
  lastUpdate = millis();

  if (chargingStripCount == 0) {
    return;  // No strips are enabled for the charging phase
  }

  const unsigned long elapsedTime = millis() - startTime;

  // Fast repeating ping-pong fill across the strip
  const uint16_t cycleMs = 1000; // 1 second there-and-back
  uint16_t phase = elapsedTime % cycleMs;
  bool descending = phase >= (cycleMs / 2);
  int half = cycleMs / 2;
  int fillProgress;
  if (!descending) {
    fillProgress = map(phase, 0, half, 0, NUM_LEDS_PER_STRIP);
  } else {
    int phase2 = phase - half;
    fillProgress = map(phase2, 0, half, NUM_LEDS_PER_STRIP, 0);
  }
  fillProgress = constrain(fillProgress, 0, NUM_LEDS_PER_STRIP);

  // Apply identical gradient and clearing logic to every charging strip
  for (size_t idx = 0; idx < chargingStripCount; ++idx) {
    CRGB* leds = chargingStrips[idx];

    // Clear first for crisp motion
    fill_solid(leds, NUM_LEDS_PER_STRIP, CRGB::Black);

    // Draw the current fill with a blue->white gradient
    for (int i = 0; i < fillProgress; ++i) {
      uint8_t blue = map(i, 0, NUM_LEDS_PER_STRIP - 1, 255, 0);
      uint8_t white = map(i, 0, NUM_LEDS_PER_STRIP - 1, 0, 255);
      leds[i] = CRGB(white, white, blue);
    }
  }
}

// Function for intense flow on strip 6 (independent climax element)
// This function runs during the second 30 seconds of the climax sequence,
// after all strips 1-5 have completed charging.
// Strip 6 uses a different effect: a flowing stream with random energy sparks.
//
// Effect:
// - 15-LED stream continuously flows around the strip
// - Color: Bright white (head) → Dim blue (tail) with smooth gradient
// - Random white sparks (30% chance per update) for intensity
// - Update rate: 50ms (fast, creates sense of urgency)
//
// This strip is independent and does NOT use the chargingEffect().
// It represents the "climax" - the peak energy release after the 5-strip buildup. (independent climax element)
void intenseFlow() {
  static unsigned long lastUpdate = 0;
  static int flowPosition = 0;
  static uint8_t hue = 0;

  if (millis() - lastUpdate > 30) {  // Very fast, intense look
    lastUpdate = millis();

#ifdef ENABLE_STRIP6
    // Base yellow glow across the entire strip (never off while active)
    const CRGB baseYellow = CRGB(90, 90, 0);
    fill_solid(strip6, NUM_LEDS_PER_STRIP, baseYellow);

    // Create a multi-color flowing stream overlay
    int streamLength = 15;  // Length of the flowing stream
    for (int i = 0; i < streamLength; i++) {
      int ledIndex = (flowPosition - i + NUM_LEDS_PER_STRIP) % NUM_LEDS_PER_STRIP;
      // Gradient from bright colorful head to dimmer tail
      uint8_t v = map(i, 0, streamLength - 1, 255, 80);
      uint8_t localHue = hue + i * 6; // color variation along the stream
      CRGB c; c.setHSV(localHue, 255, v);
      // Add on top of base yellow (saturating add)
      strip6[ledIndex] += c;
    }

    // Add frequent colorful sparks
    if (random(10) < 4) {  // 40% chance per frame
      int sparkIndex = random(NUM_LEDS_PER_STRIP);
      CRGB spark; spark.setHSV(hue + random8(), 200, 255);
      strip6[sparkIndex] += spark; // overlay spark
    }

    flowPosition = (flowPosition + 1) % NUM_LEDS_PER_STRIP;
    hue += 3; // slowly shift hue for diversity
#endif
  }
}