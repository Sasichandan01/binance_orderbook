#include "execution_engine.h"

#include <algorithm>
#include <chrono>

ExecutionEngine::ExecutionEngine(OrderBook& book, OrderManager& orderManager)
    : book_(book), orderManager_(orderManager) {}

ExecutionReport ExecutionEngine::placeOrder(Order order) {
  if (order.side == Side::Buy) {
    return executeBuy(order);
  }

  return executeSell(order);
}

ExecutionReport ExecutionEngine::executeBuy(Order& order) {
  ExecutionReport report;

  report.orderId = order.orderId;
  report.requestedQty = order.quantity;

  int64_t remaining = order.quantity;

  auto asks = book_.getAsks(10000);

  int64_t totalCost = 0;

  for (const auto& level : asks) {
    if (remaining <= 0) {
      break;
    }

    int64_t price = level.price;
    int64_t qty = level.quantity;

    if (order.type == OrderType::Limit && price > order.price) {
      break;
    }

    int64_t fillQty = std::min(remaining, qty);

    remaining -= fillQty;

    totalCost += fillQty * price;

    Trade trade;

    trade.tradeId = nextTradeId_++;
    trade.orderId = order.orderId;
    trade.price = price;
    trade.quantity = fillQty;

    trade.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();

    report.trades.push_back(trade);
  }

  report.filledQty = order.quantity - remaining;
  report.remainingQty = remaining;

  if (report.filledQty > 0) {
    report.averagePrice = static_cast<double>(totalCost) / report.filledQty;
  }

  if (remaining == 0) {
    report.status = OrderStatus::Filled;
  } else if (report.filledQty > 0) {
    report.status = OrderStatus::PartiallyFilled;

    order.filledQty = report.filledQty;
    order.status = OrderStatus::PartiallyFilled;

    orderManager_.addOrder(order);
  } else {
    report.status = OrderStatus::Open;

    order.status = OrderStatus::Open;

    orderManager_.addOrder(order);
  }

  return report;
}

ExecutionReport ExecutionEngine::executeSell(Order& order) {
  ExecutionReport report;

  report.orderId = order.orderId;
  report.requestedQty = order.quantity;

  int64_t remaining = order.quantity;

  auto bids = book_.getBids(10000);

  int64_t totalValue = 0;

  for (const auto& level : bids) {
    if (remaining <= 0) {
      break;
    }

    int64_t price = level.price;
    int64_t qty = level.quantity;

    if (order.type == OrderType::Limit && price < order.price) {
      break;
    }

    int64_t fillQty = std::min(remaining, qty);

    remaining -= fillQty;

    totalValue += fillQty * price;

    Trade trade;

    trade.tradeId = nextTradeId_++;
    trade.orderId = order.orderId;
    trade.price = price;
    trade.quantity = fillQty;

    trade.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();

    report.trades.push_back(trade);
  }

  report.filledQty = order.quantity - remaining;
  report.remainingQty = remaining;

  if (report.filledQty > 0) {
    report.averagePrice = static_cast<double>(totalValue) / report.filledQty;
  }

  if (remaining == 0) {
    report.status = OrderStatus::Filled;
  } else if (report.filledQty > 0) {
    report.status = OrderStatus::PartiallyFilled;

    order.filledQty = report.filledQty;
    order.status = OrderStatus::PartiallyFilled;

    orderManager_.addOrder(order);
  } else {
    report.status = OrderStatus::Open;

    order.status = OrderStatus::Open;

    orderManager_.addOrder(order);
  }

  return report;
}