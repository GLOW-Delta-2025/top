# DMX Multi‑Universe: States and Execution Order

Applies to: /top/big_lamps_prototype/main.cpp

Overview
- Libraries: TeensyDMX (Sender on Serial1/2/3), Arduino core.
- Universes: U1, U2, U3 (U3 initialized but not used in loop).
- Behavior: Brightness sweeps 0→255→0; U1 RGB follow brightness; U2 uses inverse red, zero green, blue follows brightness.

Key Variables
- brightness (uint8_t): 0..255, the current fade level.
- direction (int): +1 when increasing, −1 when decreasing.
- baseAddr (int): 1, base DMX address for the first fixture in U1.

High-Level States
1) Setup
   - Serial.begin(115200)
   - dmxX.begin() for U1, U2, U3
   - For channels 1..512 on each universe: set value = 0
   - Log: "DMX Multi-Universe Started"
2) Running
   - Every loop tick (~10 ms):
     - Write U1 channels:
       - Ch 1 (R) = brightness
       - Ch 2 (G) = brightness
       - Ch 3 (B) = brightness
     - Write U2 channels:
       - Ch 1 (R) = 255 - brightness
       - Ch 2 (G) = 0
       - Ch 3 (B) = brightness
     - Update brightness: brightness += direction
     - Edge handling:
       - If brightness == 255 → direction = −1
       - If brightness == 0 → direction = +1
     - delay(10)

Execution Order Per Loop Iteration
1) Compute baseAddr = 1 (constant in this sketch).
2) Set DMX for U1 (Ch 1..3) and U2 (Ch 1..3) based on current brightness.
3) Increment brightness by direction.
4) If at bounds (0 or 255), flip direction.
5) Wait 10 ms.

Timing Notes
- Update rate: ~100 Hz.
- One full triangle wave cycle: 256 steps up + 256 down = 512 steps ≈ 5.12 s.

DMX Universe/Channel Mapping Used
- Universe 1
  - Ch 1: R = brightness
  - Ch 2: G = brightness
  - Ch 3: B = brightness
- Universe 2
  - Ch 1: R = 255 - brightness
  - Ch 2: G = 0
  - Ch 3: B = brightness
- Universe 3
  - Initialized, not updated in loop.

Notes
- DMX channels are 1-based (1..512).
- FIXTURE_CHANNELS and NUM_FIXTURES_U1/U2 are defined for context but not used in loop.
- Direction flip happens exactly at 0 and 255, avoiding under/overflow artifacts.

State Diagram

~~~mermaid
stateDiagram-v2
  [*] --> Setup
  Setup: Serial + DMX init\nZero all channels
  Setup --> Running

  state Running {
    [*] --> Increasing
    Increasing: direction = +1
    Increasing --> Decreasing: brightness == 255
    Decreasing: direction = -1
    Decreasing --> Increasing: brightness == 0
  }

  Running --> Running: Each ~10 ms tick\n- Set U1/U2 channels\n- brightness += direction
~~~