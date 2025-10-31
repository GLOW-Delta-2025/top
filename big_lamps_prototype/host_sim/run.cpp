#include "include/Arduino.h"
#include "include/TeensyDMX.h"

// Trick: include the firmware after our stubs so it uses them
#define ARDUINO_TEENSY41 1
#define CORE_TEENSY 1

// Adjust the include path expectation by placing headers in include/
// and including the original file here.
#include "../main_updated.cpp"

int main() {
  // Run setup once
  setup();
  // Execute a few hundred frames
  for (int i=0; i<300; ++i) {
    loop();
  }
  // Print a simple verification for mirroring
  uint8_t u1 = qindesign::teensydmx::Sender(0).get(1); // Not accessible; rely on our logging output instead
  (void)u1;
  Serial.println("Host sim finished.");
  return 0;
}
