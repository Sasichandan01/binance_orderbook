#ifndef EXECUTION_REPORT_H
#define EXECUTION_REPORT_H

#include <vector>

#include "order.h"
#include "trade.h"

struct ExecutionReport {
  uint64_t orderId = 0;

  OrderStatus status = OrderStatus::Rejected;

  int64_t requestedQty = 0;

  int64_t filledQty = 0;

  int64_t remainingQty = 0;

  double averagePrice = 0.0;

  std::vector<Trade> trades;
};

#endif