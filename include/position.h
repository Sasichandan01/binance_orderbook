#ifndef POSITION_H
#define POSITION_H

#include <cstdint>

struct Position {
  int64_t netQuantity = 0;

  double averagePrice = 0.0;

  double realizedPnL = 0.0;

  double unrealizedPnL = 0.0;
};

#endif