#ifndef MATCHING_ENGINE_H
#define MATCHING_ENGINE_H

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "order_book.h"
#include "types.h"

enum class OrderType { LIMIT, MARKET, IOC, FOK };
enum class Side { BUY, SELL };
enum class OrderStatus { OPEN, FILLED, PARTIAL, CANCELLED, REJECTED };

struct Order {
  uint64_t orderId;
  Side side;
  OrderType type;
  int64_t price;     // scaled int64_t, 0 for market orders
  int64_t quantity;  // scaled
  int64_t filledQty;
  OrderStatus status;
  int64_t timestamp;
};

struct Fill {
  uint64_t orderId;
  int64_t price;
  int64_t quantity;
  int64_t timestamp;
};

using FillCallback = std::function<void(const Fill&)>;

class MatchingEngine {
 public:
  MatchingEngine(OrderBook& book);

  // Place an order, returns orderId
  uint64_t placeOrder(Side side, OrderType type, int64_t price,
                      int64_t quantity);

  // Cancel an order
  bool cancelOrder(uint64_t orderId);

  // Set callback for fills
  void onFill(FillCallback cb);

  // Get order by id
  const Order* getOrder(uint64_t orderId) const;

 private:
  OrderBook& book_;
  std::map<uint64_t, Order> orders_;
  uint64_t nextOrderId_ = 1;
  FillCallback fillCallback_;

  void matchLimit(Order& order);
  void matchMarket(Order& order);
  void matchIOC(Order& order);
  void matchFOK(Order& order);
  void executeFill(Order& order, int64_t price, int64_t qty);
};

#endif