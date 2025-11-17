/*
  Single LED Strip Demo

  This demo controls one WS2812B LED strip using 3 wires:
  - 5V (power)
  - GND (ground)
  - Data (GPIO pin)

  You CANNOT control addressable LEDs with just power and ground.
  The data line is required to send signals that determine which LEDs light up
  and in what colors.

  For non-addressable strips, you could control them with just power/ground,
  but those are typically single-color and controlled by switching power on/off.
*/

#include <FastLED.h>

#define LED_PIN     2    // GPIO pin for data line
#define NUM_LEDS    10   // Number of LEDs in your strip (adjust as needed)
#define BRIGHTNESS  64   // Brightness level (0-255)

CRGB leds[NUM_LEDS];

void setup() {
  Serial.begin(115200);

  // Initialize FastLED
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  Serial.println("Single LED Strip Demo Started");
  Serial.println("Make sure you have 3 connections:");
  Serial.println("- 5V to strip power");
  Serial.println("- GND to strip ground");
  Serial.println("- GPIO 2 to strip data input");
}

void loop() {
  // Demo 1: Fill strip with red
  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.show();
  delay(2000);

  // Demo 2: Fill strip with green
  fill_solid(leds, NUM_LEDS, CRGB::Green);
  FastLED.show();
  delay(2000);

  // Demo 3: Fill strip with blue
  fill_solid(leds, NUM_LEDS, CRGB::Blue);
  FastLED.show();
  delay(2000);

  // Demo 4: Rainbow effect
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(i * 255 / NUM_LEDS, 255, 255);
  }
  FastLED.show();
  delay(2000);

  // Demo 5: Chase effect
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::White;
    FastLED.show();
    delay(100);
    leds[i] = CRGB::Black;
  }

  // Demo 6: All off
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  delay(1000);
}