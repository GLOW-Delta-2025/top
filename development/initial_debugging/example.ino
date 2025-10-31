/*
  Blink All LEDs

  Demonstrates usage of multiple LEDs on ESP board, including RGB LED.

  This code controls multiple GPIO LEDs and the onboard RGB LED to blink in a rhythm.
*/

// Define LED pins (adjust based on your ESP board)
const int ledPins[] = {2, 4, 5, 12, 13, 14, 15}; // GPIO pins with LEDs
const int numLeds = sizeof(ledPins) / sizeof(ledPins[0]);

#define RGB_BRIGHTNESS 64 // Change white brightness (max 255)

// the setup function runs once when you press reset or power the board
void setup() {
  // Initialize GPIO LED pins as outputs
  for (int i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW); // Start with LEDs off
  }
  // No need to initialize the RGB LED
}

// the loop function runs over and over again forever
void loop() {
  // Blink GPIO LEDs in sequence (chase effect)
  for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
    delay(200);
    digitalWrite(ledPins[i], LOW);
  }

  // Control RGB LED if available
#ifdef RGB_BUILTIN
  digitalWrite(RGB_BUILTIN, HIGH);  // Turn the RGB LED white
  delay(500);
  digitalWrite(RGB_BUILTIN, LOW);  // Turn the RGB LED off
  delay(500);

  rgbLedWrite(RGB_BUILTIN, RGB_BRIGHTNESS, 0, 0);  // Red
  delay(500);
  rgbLedWrite(RGB_BUILTIN, 0, RGB_BRIGHTNESS, 0);  // Green
  delay(500);
  rgbLedWrite(RGB_BUILTIN, 0, 0, RGB_BRIGHTNESS);  // Blue
  delay(500);
  rgbLedWrite(RGB_BUILTIN, 0, 0, 0);  // Off / black
  delay(500);
#endif

  // All LEDs off for a moment
  delay(1000);
}
