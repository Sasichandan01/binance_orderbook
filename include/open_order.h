#ifndef OPEN_ORDER_H
#define OPEN_ORDER_H

#include "order.h"

struct OpenOrder {
  Order order;

  int64_t remainingQty = 0;
};

#endif