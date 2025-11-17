#pragma once
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <iostream>

using uint8_t = std::uint8_t;
using uint16_t = std::uint16_t;
using uint32_t = std::uint32_t;

static inline uint32_t millis();

// Simulated clock
static inline uint32_t &__sim_millis_ref() {
  static uint32_t g = 0;
  return g;
}

static inline uint32_t millis() { return __sim_millis_ref(); }

static inline void delay(uint32_t ms) { __sim_millis_ref() += ms; }

struct SerialClass {
  void begin(unsigned long) {}
  void print(const char *s) { std::cout << s; }
  void print(uint32_t v) { std::cout << v; }
  void print(int v) { std::cout << v; }
  void print(uint8_t v) { std::cout << (unsigned)v; }
  void println(const char *s) { std::cout << s << '\n'; }
  void println(uint32_t v) { std::cout << v << '\n'; }
  void println(int v) { std::cout << v << '\n'; }
  void println(uint8_t v) { std::cout << (unsigned)v << '\n'; }
  void printf(const char *fmt, ...) {
    va_list args; va_start(args, fmt); vprintf(fmt, args); va_end(args);
  }
} ;

static SerialClass Serial;

// Minimal elapsedMillis compatible with Teensy semantics
class elapsedMillis {
public:
  elapsedMillis() : start_(millis()) {}
  operator unsigned long() const { return millis() - start_; }
  elapsedMillis &operator=(unsigned long val) { start_ = millis() - val; return *this; }
private:
  unsigned long start_;
};
