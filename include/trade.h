#ifndef TRADE_H
#define TRADE_H

#include <cstdint>

struct Trade {
  uint64_t tradeId = 0;

  uint64_t orderId = 0;

  int64_t price = 0;
  int64_t quantity = 0;

  uint64_t timestamp = 0;
};

#endif