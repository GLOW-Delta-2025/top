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
// #define ENABLE_STRIP1
// #define ENABLE_STRIP2
// #define ENABLE_STRIP3
// #define ENABLE_STRIP4
// #define ENABLE_STRIP5
// #define ENABLE_STRIP6

// TESTING CONFIGURATION (currently active):
// Only strips 1 and 6 enabled. Strips 1-5 will be synchronized when all are enabled.
#define ENABLE_STRIP1  // Main 5-strip array (charging effect)
//#define ENABLE_STRIP2
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

// Timing variables for the climax sequence
// Sequence phases:
// 1. Charging phase (flashDuration): Strips 1-5 progressively fill with blue→white gradient
// 2. Flow phase (burstDuration): Strip 6 activates with intense animated stream
unsigned long startTime;
unsigned long fillDuration = 30000;  // 30 seconds to fill strips 1-5 (legacy variable, not used)
unsigned long strip6Delay = 5000;    // 5 seconds delay before strip 6 (legacy variable, not used)
unsigned long flashDuration = 30000;  // 30 seconds charging for strips 1-5
unsigned long burstDuration = 30000;  // 30 seconds for intense flow on strip 6
bool strips1to5Filled = false;
bool strip6Active = false;
bool sequenceActive = false;
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

void loop() {
  // Check for serial commands from both USB (Serial) and external TX/RX (Serial2)
  checkSerialCommands();

  unsigned long currentTime = millis();

  // Main state machine: if sequence is active, run the climax animation
  if (sequenceActive) {
    unsigned long elapsedTime = currentTime - startTime;

    // PHASE 1: CHARGING (0-30 seconds)
    // Strips 1-5: Progressive fill from blue to white
    // This phase creates the "buildup" effect, with all 5 strips synchronized.
    if (!strip1Flashing && elapsedTime < flashDuration) {
      strip1Flashing = true;
      Serial.println("Starting strip 1 charging");
    }

    if (strip1Flashing && elapsedTime < flashDuration) {
      chargingEffect();  // This updates strip 1 (and will update 2-5 once they're enabled)
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

    // End sequence: turn off all strips and signal completion
    if (elapsedTime >= flashDuration + burstDuration) {
      sequenceActive = false;
      strip1Flashing = false;
      strip6Bursting = false;
      // Turn off all strips
#ifdef ENABLE_STRIP1
      fill_solid(strip1, NUM_LEDS_PER_STRIP, CRGB::Black);
#endif
#ifdef ENABLE_STRIP6
      fill_solid(strip6, NUM_LEDS_PER_STRIP, CRGB::Black);
#endif
      FastLED.show();
     Serial.println("Climax sequence completed");
  // Send completion confirmation to both Serial and Serial2
  Serial.println("!!MASTER:CONFIRM:STOP_CLIMAX_TOP##");
  Serial2.println("!!MASTER:CONFIRM:STOP_CLIMAX_TOP##");
    }
  } else {
    // Idle state - keep strips off
#ifdef ENABLE_STRIP1
    fill_solid(strip1, NUM_LEDS_PER_STRIP, CRGB::Black);
#endif
#ifdef ENABLE_STRIP6
    fill_solid(strip6, NUM_LEDS_PER_STRIP, CRGB::Black);
#endif
    FastLED.show();
  }

  FastLED.show();
  delay(50);
}

// Function to check for serial commands
void checkSerialCommands() {
  while (Serial.available()) {
    char c = Serial.read();
    serialBuffer += c;

    // Check if we have a complete command
    if (serialBuffer.endsWith("##")) {
      if (serialBuffer.startsWith("!!TOP:REQUEST:START_CLIMAX_TOP")) {
        // Extract time if needed (for future use)
        int startIdx = serialBuffer.indexOf('{');
        int endIdx = serialBuffer.indexOf('}');
        if (startIdx != -1 && endIdx != -1) {
          String timeStr = serialBuffer.substring(startIdx + 1, endIdx);
          Serial.println("Received start climax command with time: " + timeStr);
        }

        // Send confirmation on both Serial and Serial2
        Serial.println("!MASTER:CONFIRM:START_CLIMAX_TOP##");
        Serial2.println("!MASTER:CONFIRM:START_CLIMAX_TOP##");

        // Start the sequence
        if (!sequenceActive) {
          sequenceActive = true;
          startTime = millis();
          Serial.println("Starting climax sequence");
        }
      } else if (serialBuffer.startsWith("!!TOP:REQUEST:STOP_CLIMAX_TOP")) {
        // Stop the sequence
        if (sequenceActive) {
          sequenceActive = false;
          strip1Flashing = false;
          strip6Bursting = false;
          // Turn off all strips
#ifdef ENABLE_STRIP1
          fill_solid(strip1, NUM_LEDS_PER_STRIP, CRGB::Black);
#endif
#ifdef ENABLE_STRIP6
          fill_solid(strip6, NUM_LEDS_PER_STRIP, CRGB::Black);
#endif
          FastLED.show();
          Serial.println("Climax sequence stopped manually");
          // Send stop confirmation to both Serial and Serial2
          Serial.println("!!MASTER:CONFIRM:STOP_CLIMAX_TOP##");
          Serial2.println("!!MASTER:CONFIRM:STOP_CLIMAX_TOP##");
        }
      }
      serialBuffer = "";  // Clear buffer
    }
  }

  // Also read from Serial2 (external TX/RX)
  while (Serial2.available()) {
    char c2 = Serial2.read();
    serial2Buffer += c2;

    if (serial2Buffer.endsWith("##")) {
      if (serial2Buffer.startsWith("!!TOP:REQUEST:START_CLIMAX_TOP")) {
        int startIdx = serial2Buffer.indexOf('{');
        int endIdx = serial2Buffer.indexOf('}');
        if (startIdx != -1 && endIdx != -1) {
          String timeStr2 = serial2Buffer.substring(startIdx + 1, endIdx);
          Serial.println("(RX2) Received start climax command with time: " + timeStr2);
        }
        // Send confirmation on both Serial and Serial2
        Serial.println("!MASTER:CONFIRM:START_CLIMAX_TOP##");
        Serial2.println("!MASTER:CONFIRM:START_CLIMAX_TOP##");
        if (!sequenceActive) {
          sequenceActive = true;
          startTime = millis();
          Serial.println("(RX2) Starting climax sequence");
        }
      } else if (serial2Buffer.startsWith("!!TOP:REQUEST:STOP_CLIMAX_TOP")) {
        if (sequenceActive) {
          sequenceActive = false;
          strip1Flashing = false;
          strip6Bursting = false;
#ifdef ENABLE_STRIP1
          fill_solid(strip1, NUM_LEDS_PER_STRIP, CRGB::Black);
#endif
#ifdef ENABLE_STRIP6
          fill_solid(strip6, NUM_LEDS_PER_STRIP, CRGB::Black);
#endif
          FastLED.show();
          Serial.println("(RX2) Climax sequence stopped manually");
          // Send stop confirmation on both ports
          Serial.println("!!MASTER:CONFIRM:STOP_CLIMAX_TOP##");
          Serial2.println("!!MASTER:CONFIRM:STOP_CLIMAX_TOP##");
        }
      }
      serial2Buffer = ""; // Clear buffer
    }
  }
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

  if (millis() - lastUpdate > 100) {  // Update every 100ms
    lastUpdate = millis();

#ifdef ENABLE_STRIP1
    unsigned long elapsedTime = millis() - startTime;
    int fillProgress = map(elapsedTime, 0, flashDuration, 0, NUM_LEDS_PER_STRIP);

    // Fill from start to current progress
    for (int i = 0; i < min(fillProgress, NUM_LEDS_PER_STRIP); i++) {
      // Gradient from blue (start) to white (end)
      uint8_t blue = map(i, 0, NUM_LEDS_PER_STRIP - 1, 255, 0);
      uint8_t white = map(i, 0, NUM_LEDS_PER_STRIP - 1, 0, 255);
      strip1[i] = CRGB(white, white, blue);
    }

    // Clear the rest
    for (int i = fillProgress; i < NUM_LEDS_PER_STRIP; i++) {
      strip1[i] = CRGB::Black;
    }
#endif

  // TODO: When enabling all 6 strips, add identical chargingEffect code for strips 2-5
  // Simply uncomment lines below
  // This ensures all 5 main strips charge in perfect synchronization.
  // Example implementation (commented out for now):
  //
  // #ifdef ENABLE_STRIP2
  //   for (int i = 0; i < min(fillProgress, NUM_LEDS_PER_STRIP); i++) {
  //     uint8_t blue = map(i, 0, NUM_LEDS_PER_STRIP - 1, 255, 0);
  //     uint8_t white = map(i, 0, NUM_LEDS_PER_STRIP - 1, 0, 255);
  //     strip2[i] = CRGB(white, white, blue);
  //   }
  //   for (int i = fillProgress; i < NUM_LEDS_PER_STRIP; i++) {
  //     strip2[i] = CRGB::Black;
  //   }
  // #endif
  //
  // #ifdef ENABLE_STRIP3
  //   for (int i = 0; i < min(fillProgress, NUM_LEDS_PER_STRIP); i++) {
  //     uint8_t blue = map(i, 0, NUM_LEDS_PER_STRIP - 1, 255, 0);
  //     uint8_t white = map(i, 0, NUM_LEDS_PER_STRIP - 1, 0, 255);
  //     strip3[i] = CRGB(white, white, blue);
  //   }
  //   for (int i = fillProgress; i < NUM_LEDS_PER_STRIP; i++) {
  //     strip3[i] = CRGB::Black;
  //   }
  // #endif
  //
  // #ifdef ENABLE_STRIP4
  //   for (int i = 0; i < min(fillProgress, NUM_LEDS_PER_STRIP); i++) {
  //     uint8_t blue = map(i, 0, NUM_LEDS_PER_STRIP - 1, 255, 0);
  //     uint8_t white = map(i, 0, NUM_LEDS_PER_STRIP - 1, 0, 255);
  //     strip4[i] = CRGB(white, white, blue);
  //   }
  //   for (int i = fillProgress; i < NUM_LEDS_PER_STRIP; i++) {
  //     strip4[i] = CRGB::Black;
  //   }
  // #endif
  //
  // #ifdef ENABLE_STRIP5
  //   for (int i = 0; i < min(fillProgress, NUM_LEDS_PER_STRIP); i++) {
  //     uint8_t blue = map(i, 0, NUM_LEDS_PER_STRIP - 1, 255, 0);
  //     uint8_t white = map(i, 0, NUM_LEDS_PER_STRIP - 1, 0, 255);
  //     strip5[i] = CRGB(white, white, blue);
  //   }
  //   for (int i = fillProgress; i < NUM_LEDS_PER_STRIP; i++) {
  //     strip5[i] = CRGB::Black;
  //   }
  // #endif
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

  if (millis() - lastUpdate > 50) {  // Faster updates for intensity
    lastUpdate = millis();

#ifdef ENABLE_STRIP6
    // Clear strip
    fill_solid(strip6, NUM_LEDS_PER_STRIP, CRGB::Black);

    // Create flowing stream effect
    int streamLength = 15;  // Length of the flowing stream
    for (int i = 0; i < streamLength; i++) {
      int ledIndex = (flowPosition - i + NUM_LEDS_PER_STRIP) % NUM_LEDS_PER_STRIP;
      // Gradient from bright white (head) to dim blue (tail)
      uint8_t brightness = map(i, 0, streamLength - 1, 255, 50);
      uint8_t blue = map(i, 0, streamLength - 1, 0, 200);
      strip6[ledIndex] = CRGB(brightness, brightness, blue);
    }

    // Add some random sparks
    if (random(10) < 3) {  // 30% chance
      int sparkIndex = random(NUM_LEDS_PER_STRIP);
      strip6[sparkIndex] = CRGB::White;
    }

    flowPosition = (flowPosition + 1) % NUM_LEDS_PER_STRIP;
#endif
  }
}