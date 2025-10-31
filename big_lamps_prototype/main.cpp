//Initial code

#include <TeensyDMX.h>
#include <Arduino.h>
using namespace qindesign::teensydmx;

// Create DMX output universes
Sender dmx1(Serial1);  // Universe 1
Sender dmx2(Serial2);  // Universe 2
Sender dmx3(Serial3);  // Universe 3
// Add more if needed

// Fixture info
const int FIXTURE_CHANNELS = 108;
const int NUM_FIXTURES_U1 = 4; // example: 4 fixtures * 108 = 432 channels
const int NUM_FIXTURES_U2 = 2; // example for universe 2

void setup() {
  Serial.begin(115200);
  dmx1.begin();
  dmx2.begin();
  dmx3.begin();

  // Initialize all channels to 0
  for (int i = 1; i <= 512; i++) {
    dmx1.set(i, 0);
    dmx2.set(i, 0);
    dmx3.set(i, 0);
  }

  Serial.println("DMX Multi-Universe Started");
}

void loop() {
  // Example: sweep brightness on fixture 1 (Universe 1)
  static uint8_t brightness = 0;
  static int direction = 1;

  // Update first fixture (red, green, blue channels)
  int baseAddr = 1;
  dmx1.set(baseAddr + 0, brightness);  // Red
  dmx1.set(baseAddr + 1, brightness);  // Green
  dmx1.set(baseAddr + 2, brightness);  // Blue

  // Example for Universe 2 (fixture starting at address 1)
  dmx2.set(1, 255 - brightness);  // Red inverse
  dmx2.set(2, 0);
  dmx2.set(3, brightness);

  brightness += direction;
  if (brightness == 0 || brightness == 255) direction = -direction;

  delay(10); // smooth fade
}
