#pragma once
#include <array>
#include <cstdint>

namespace qindesign { namespace teensydmx {

class Sender {
public:
  Sender(int) {}
  void begin() { }
  void clear() { buffer.fill(0); }
  void set(int ch, uint8_t val) { if (ch>=1 && ch<=512) buffer[ch-1]=val; }
  uint8_t get(int ch) const { return (ch>=1 && ch<=512)? buffer[ch-1]:0; }
private:
  std::array<uint8_t,512> buffer{};
};

}}

// Simulate Serial1/Serial2 identifiers
static const int Serial1 = 1;
static const int Serial2 = 2;
