#ifndef TYPES_H
#define TYPES_H

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

constexpr int64_t PRICE_SCALE = 100000000LL;

inline double fromScaled(int64_t v) {
  return static_cast<double>(v) / PRICE_SCALE;
}

inline int64_t toScaled(double v) {
  return static_cast<int64_t>(std::llround(v * PRICE_SCALE));
}

struct PriceLevel {
  int64_t price = 0;

  int64_t quantity = 0;
};

struct DepthDelta {
  uint64_t firstUpdateId = 0;

  uint64_t finalUpdateId = 0;

  uint64_t eventTime = 0;

  std::vector<PriceLevel> bids;

  std::vector<PriceLevel> asks;
};

struct BookSnapshot {
  uint64_t lastUpdateId = 0;

  uint64_t timestamp = 0;

  std::vector<PriceLevel> bids;

  std::vector<PriceLevel> asks;
};

#endif