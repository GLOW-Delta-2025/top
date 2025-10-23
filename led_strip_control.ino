#include <FastLED.h>

// LED Strip Configuration
#define NUM_LEDS_PER_STRIP 60  // Adjust based on your strip length
#define BRIGHTNESS 64

// Pin assignments for each strip
#define STRIP1_PIN 2
#define STRIP2_PIN 4
#define STRIP3_PIN 5
#define STRIP4_PIN 12
#define STRIP5_PIN 13
#define STRIP6_PIN 14

// Enable/disable strips (comment out to disable)
#define ENABLE_STRIP1  // This represents all 5 strips acting the same
//#define ENABLE_STRIP2
//#define ENABLE_STRIP3
//#define ENABLE_STRIP4
//#define ENABLE_STRIP5
#define ENABLE_STRIP6  // The sixth element

// Serial command
#define UART_RX_PIN 16  // RX pin for external commands (Serial2)
#define UART_TX_PIN 17  // TX pin for external responses (Serial2)
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

// Timing variables
unsigned long startTime;
unsigned long fillDuration = 30000;  // 30 seconds to fill strips 1-5
unsigned long strip6Delay = 5000;    // 5 seconds delay before strip 6
unsigned long flashDuration = 30000;  // 30 seconds charging for strip 1
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
  Serial2.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  Serial.println("Serial2 initialized on RX=" + String(UART_RX_PIN) + ", TX=" + String(UART_TX_PIN));

  // Initialize FastLED for each enabled strip
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
  // Check for serial commands
  checkSerialCommands();

  unsigned long currentTime = millis();

  if (sequenceActive) {
    unsigned long elapsedTime = currentTime - startTime;

    // Strip 1: Charging effect for 30 seconds
    if (!strip1Flashing && elapsedTime < flashDuration) {
      strip1Flashing = true;
      Serial.println("Starting strip 1 charging");
    }

    if (strip1Flashing && elapsedTime < flashDuration) {
      chargingEffect();
    }

    // Transition to strip 6 intense flow
    if (elapsedTime >= flashDuration) {
      strip6Bursting = true;
      strip1Flashing = false;
      Serial.println("Starting strip 6 intense flow");
    }

    // Strip 6: Intense flow
    if (strip6Bursting && elapsedTime < flashDuration + burstDuration) {
      intenseFlow();
    }

    // End sequence
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

// Function for charging effect on strip 1
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
  }
}

// Function for intense flow on strip 6
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