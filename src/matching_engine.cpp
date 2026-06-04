#include "matching_engine.h"

#include <spdlog/spdlog.h>

#include <chrono>

static int64_t now_ms() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

MatchingEngine::MatchingEngine(OrderBook& book) : book_(book) {}

void MatchingEngine::onFill(FillCallback cb) { fillCallback_ = cb; }

uint64_t MatchingEngine::placeOrder(Side side, OrderType type, int64_t price,
                                    int64_t quantity) {
  Order order;
  order.orderId = nextOrderId_++;
  order.side = side;
  order.type = type;
  order.price = price;
  order.quantity = quantity;
  order.filledQty = 0;
  order.status = OrderStatus::OPEN;
  order.timestamp = now_ms();

  orders_[order.orderId] = order;
  auto& o = orders_[order.orderId];

  switch (type) {
    case OrderType::LIMIT:
      matchLimit(o);
      break;
    case OrderType::MARKET:
      matchMarket(o);
      break;
    case OrderType::IOC:
      matchIOC(o);
      break;
    case OrderType::FOK:
      matchFOK(o);
      break;
  }

  spdlog::info("Order {}: side={} type={} price={} qty={} status={}", o.orderId,
               o.side == Side::BUY ? "BUY" : "SELL", (int)o.type, o.price,
               o.quantity, (int)o.status);

  return o.orderId;
}

void MatchingEngine::executeFill(Order& order, int64_t price, int64_t qty) {
  order.filledQty += qty;
  if (order.filledQty >= order.quantity)
    order.status = OrderStatus::FILLED;
  else
    order.status = OrderStatus::PARTIAL;

  Fill fill{order.orderId, price, qty, now_ms()};
  if (fillCallback_) fillCallback_(fill);

  spdlog::info("Fill: orderId={} price={} qty={}", fill.orderId, fill.price,
               fill.quantity);
}

void MatchingEngine::matchLimit(Order& order) {
  int64_t remaining = order.quantity - order.filledQty;

  if (order.side == Side::BUY) {
    // Match against asks: fill if ask price <= our limit price
    auto asks = book_.getAsks(50);
    for (auto& [askPrice, askQty] : asks) {
      if (askPrice > order.price) break;
      int64_t fillQty = std::min(remaining, askQty);
      executeFill(order, askPrice, fillQty);
      remaining -= fillQty;
      if (remaining <= 0) break;
    }
  } else {
    // Match against bids: fill if bid price >= our limit price
    auto bids = book_.getBids(50);
    for (auto& [bidPrice, bidQty] : bids) {
      if (bidPrice < order.price) break;
      int64_t fillQty = std::min(remaining, bidQty);
      executeFill(order, bidPrice, fillQty);
      remaining -= fillQty;
      if (remaining <= 0) break;
    }
  }
  // Unfilled portion stays OPEN (resting limit order)
  if (order.filledQty == 0) order.status = OrderStatus::OPEN;
}

void MatchingEngine::matchMarket(Order& order) {
  int64_t remaining = order.quantity - order.filledQty;

  if (order.side == Side::BUY) {
    auto asks = book_.getAsks(50);
    for (auto& [askPrice, askQty] : asks) {
      int64_t fillQty = std::min(remaining, askQty);
      executeFill(order, askPrice, fillQty);
      remaining -= fillQty;
      if (remaining <= 0) break;
    }
  } else {
    auto bids = book_.getBids(50);
    for (auto& [bidPrice, bidQty] : bids) {
      int64_t fillQty = std::min(remaining, bidQty);
      executeFill(order, bidPrice, fillQty);
      remaining -= fillQty;
      if (remaining <= 0) break;
    }
  }
  if (order.filledQty < order.quantity) order.status = OrderStatus::PARTIAL;
}

void MatchingEngine::matchIOC(Order& order) {
  // Same as limit but cancel unfilled portion immediately
  matchLimit(order);
  if (order.status == OrderStatus::OPEN || order.status == OrderStatus::PARTIAL)
    order.status = OrderStatus::CANCELLED;
}

void MatchingEngine::matchFOK(Order& order) {
  // Check if full quantity can be filled before executing
  int64_t available = 0;

  if (order.side == Side::BUY) {
    for (auto& [p, q] : book_.getAsks(50)) {
      if (p > order.price) break;
      available += q;
      if (available >= order.quantity) break;
    }
  } else {
    for (auto& [p, q] : book_.getBids(50)) {
      if (p < order.price) break;
      available += q;
      if (available >= order.quantity) break;
    }
  }

  if (available < order.quantity) {
    order.status = OrderStatus::REJECTED;
    spdlog::warn("FOK order {} rejected - insufficient liquidity",
                 order.orderId);
    return;
  }
  matchLimit(order);
}

bool MatchingEngine::cancelOrder(uint64_t orderId) {
  auto it = orders_.find(orderId);
  if (it == orders_.end()) return false;
  if (it->second.status != OrderStatus::OPEN) return false;
  it->second.status = OrderStatus::CANCELLED;
  spdlog::info("Order {} cancelled", orderId);
  return true;
}

const Order* MatchingEngine::getOrder(uint64_t orderId) const {
  auto it = orders_.find(orderId);
  return it == orders_.end() ? nullptr : &it->second;
}