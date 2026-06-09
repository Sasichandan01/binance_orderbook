#include "execution_engine.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>

ExecutionEngine::ExecutionEngine(OrderBook& book, OrderManager& orderManager)
    : book_(book), orderManager_(orderManager) {}

ExecutionReport ExecutionEngine::placeOrder(Order order) {
  auto logger = ExecutionLogger::get();

  logger->info(
      "[NEW_ORDER] "
      "id={} "
      "side={} "
      "qty={} "
      "price={} "
      "type={}",

      order.orderId,

      order.side == Side::Buy ? "BUY" : "SELL",

      order.quantity,

      order.price,

      order.type == OrderType::Market ? "MARKET" : "LIMIT");
  if (order.side == Side::Buy) {
    return executeBuy(order);
  }

  return executeSell(order);
}

ExecutionReport ExecutionEngine::matchBuy(Order& order) {
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

    if (order.type == OrderType::Limit && level.price > order.price) {
      break;
    }

    int64_t fillQty = std::min(remaining, level.quantity);

    remaining -= fillQty;

    totalCost += fillQty * level.price;

    Trade trade;

    trade.tradeId = nextTradeId_++;
    trade.orderId = order.orderId;
    trade.price = level.price;
    trade.quantity = fillQty;

    trade.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();

    report.trades.push_back(trade);
    auto logger = ExecutionLogger::get();

    logger->info(
        "[TRADE] "
        "orderId={} "
        "Side=BUY"
        "price={} "
        "qty={}",

        trade.orderId, trade.price, trade.quantity);
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
  } else {
    report.status = OrderStatus::Open;
  }

  return report;
}

ExecutionReport ExecutionEngine::matchSell(Order& order) {
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

    if (order.type == OrderType::Limit && level.price < order.price) {
      break;
    }

    int64_t fillQty = std::min(remaining, level.quantity);

    remaining -= fillQty;

    totalValue += fillQty * level.price;

    Trade trade;

    trade.tradeId = nextTradeId_++;
    trade.orderId = order.orderId;
    trade.price = level.price;
    trade.quantity = fillQty;

    trade.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();

    report.trades.push_back(trade);
    auto logger = ExecutionLogger::get();

    logger->info(
        "[TRADE] "
        "orderId={} "
        "Side=SELL"
        "price={} "
        "qty={}",

        trade.orderId, trade.price, trade.quantity);
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
  } else {
    report.status = OrderStatus::Open;
  }

  return report;
}

ExecutionReport ExecutionEngine::executeBuy(Order& order) {
  ExecutionReport report = matchBuy(order);

  if (report.status == OrderStatus::Filled) {
    return report;
  }

  order.filledQty = report.filledQty;
  order.status = report.status;

  orderManager_.addOrder(order);

  return report;
}

ExecutionReport ExecutionEngine::executeSell(Order& order) {
  ExecutionReport report = matchSell(order);

  if (report.status == OrderStatus::Filled) {
    return report;
  }

  order.filledQty = report.filledQty;
  order.status = report.status;

  orderManager_.addOrder(order);

  return report;
}

void ExecutionEngine::processOpenOrders() {
  auto& openOrders = orderManager_.getOpenOrders();

  std::vector<uint64_t> completedOrders;

  for (auto& [orderId, open] : openOrders) {
    Order workingOrder = open.order;

    workingOrder.quantity = open.remainingQty;

    ExecutionReport report;

    if (workingOrder.side == Side::Buy) {
      report = matchBuy(workingOrder);
    } else {
      report = matchSell(workingOrder);
    }

    if (report.filledQty <= 0) {
      continue;
    }

    open.order.filledQty += report.filledQty;

    open.remainingQty -= report.filledQty;

    ExecutionLogger::get()->info(
        "[OPEN_ORDER_FILL] "
        "orderId={} "
        "filled={} "
        "remaining={}",

        orderId, report.filledQty, open.remainingQty);

    if (open.remainingQty <= 0) {
      open.order.status = OrderStatus::Filled;

      completedOrders.push_back(orderId);

      ExecutionLogger::get()->info(
          "[ORDER_COMPLETED] "
          "orderId={} "
          "totalFilled={}",

          orderId, open.order.filledQty);
    } else {
      open.order.status = OrderStatus::PartiallyFilled;
    }
  }

  for (auto orderId : completedOrders) {
    orderManager_.removeOrder(orderId);
  }
}