#ifndef ORDER_H
#define ORDER_H

#include <cstdint>

enum class Side { Buy, Sell };

enum class OrderType { Market, Limit };

enum class OrderStatus {
  New,
  Open,
  PartiallyFilled,
  Filled,
  Cancelled,
  Rejected
};

struct Order {
  uint64_t orderId = 0;

  Side side;
  OrderType type;

  int64_t price = 0;     // scaled
  int64_t quantity = 0;  // original qty

  int64_t filledQty = 0;

  OrderStatus status = OrderStatus::New;

  uint64_t timestamp = 0;
};

#endif