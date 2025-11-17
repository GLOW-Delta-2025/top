# ESP LED Strip Installation Plan

## Overview
This plan outlines the setup for controlling 6 LED strips using an ESP32 board:
- Strips 1-5: Work together as the main installation
- Strip 6: Activates after strips 1-5 have been running for a set time and are "filled" (fully lit)

## Hardware Setup

### Recommended Approach: Separate Pins for Independent Control
Using separate GPIO pins for each strip allows for maximum flexibility and independent control. This is better than connecting strips in series or parallel on the same pin, as it enables:
- Individual strip control
- Easier synchronization through code
- Ability to handle different strip lengths
- Better fault isolation

### Pin Assignments
- Strip 1: GPIO 2
- Strip 2: GPIO 4
- Strip 3: GPIO 5
- Strip 4: GPIO 12
- Strip 5: GPIO 13
- Strip 6: GPIO 14

### Why Not Connect All to Same Pin?
- **Series Connection**: Would treat all strips as one continuous strip, losing individual control
- **Parallel Connection**: Not recommended for addressable LEDs as data signals would interfere
- **Separate Pins**: Allows each strip to be controlled independently while still coordinating them via software

### Power Considerations
- Each LED strip requires significant power (typically 60mA per meter at full white)
- Use a separate power supply for the strips (5V or 12V depending on strip type)
- Connect ESP32 ground to strip power supply ground
- Consider power injection for longer strips to prevent voltage drop

### Strip Specifications
- Assume WS2812B or similar addressable RGB LED strips
- Each strip: 1 meter, 60 LEDs/meter (adjust in code as needed)
- Data connection: 3-pin JST or direct wire to ESP GPIO

## Software Implementation

### Library
Use FastLED library for ESP32 Arduino environment.

### Conditional Strip Control
The code includes conditional compilation flags to enable/disable individual strips:
```cpp
#define ENABLE_STRIP1  // Comment out to disable
#define ENABLE_STRIP6
```
This allows you to gradually connect strips without modifying the main logic.

### Serial Commands
The ESP accepts these commands over TX/RX (serial):

**Start Command:**
```
!!TOP:REQUEST:START_CLIMAX_TOP{[TIME]}##
```

**Stop Command:**
```
!!TOP:REQUEST:STOP_CLIMAX_TOP##
```

### Confirmations (Emitted by ESP)
- On receiving START command:
  ```
  !MASTER:CONFIRM:START_CLIMAX_TOP##
  ```
- On sequence completion or manual stop:
  ```
  !!MASTER:CONFIRM:STOP_CLIMAX_TOP##
  ```

### Command Responses
- **Start**: Begins the climax sequence
- **Stop**: Immediately stops the sequence and turns off strips
- **Completion**: When sequence ends naturally, ESP sends:
  ```
  !!TOP:REQUEST:STOP_CLIMAX_TOP##
  ```

This allows for debugging and external control of the light installation.

### Control Logic
1. **Idle State**: Strips remain off until command received
2. **Strip 1 (30 seconds)**: Charging effect - LEDs fill progressively from blue to white
3. **Strip 6 (30 seconds)**: Intense flow - flowing stream of LEDs with random sparks
4. **Completion**: All strips turn off, ready for next command

### Timing
- Charging duration: 30 seconds
- Flow duration: 30 seconds
- Charging update: 100ms
- Flow update: 50ms (faster for intensity)

## Files
- `led_strip_control.ino`: Main Arduino sketch
- `led_strip_plan.md`: This plan document

## Next Steps
1. Gather hardware components
2. Install FastLED library in Arduino IDE
3. Test individual strips
4. Implement and test code
5. Adjust timing and patterns as needed