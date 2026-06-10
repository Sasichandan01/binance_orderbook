#include "execution_engine.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>

#include "async_logger.h"

extern std::unique_ptr<AsyncLogger> g_logger;
ExecutionEngine::ExecutionEngine(OrderBook& book, OrderManager& orderManager)
    : book_(book), orderManager_(orderManager) {}

ExecutionReport ExecutionEngine::placeOrder(Order order) {
  spdlog::info(
      "[NEW_ORDER] "
      "orderId={} "
      "side={} "
      "type={} "
      "price={} "
      "qty={}",

      order.orderId, order.side == Side::Buy ? "BUY" : "SELL",
      order.type == OrderType::Market ? "MARKET" : "LIMIT",
      static_cast<double>(order.price) / PRICE_SCALE,
      static_cast<double>(order.quantity) / PRICE_SCALE);

  if (g_logger) {
    g_logger->logOrder(
        "[NEW_ORDER] orderId=" + std::to_string(order.orderId) + " side=" +
        std::string(order.side == Side::Buy ? "BUY" : "SELL") + " type=" +
        std::string(order.type == OrderType::Market ? "MARKET" : "LIMIT") +
        " price=" +
        std::to_string(static_cast<double>(order.price) / PRICE_SCALE) +
        " qty=" +
        std::to_string(static_cast<double>(order.quantity) / PRICE_SCALE));
  }

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

  __int128 totalCost = 0;

  for (const auto& level : asks) {
    if (remaining <= 0) {
      break;
    }

    if (order.type == OrderType::Limit && level.price > order.price) {
      break;
    }

    int64_t fillQty = std::min(remaining, level.quantity);

    remaining -= fillQty;

    totalCost +=
        static_cast<__int128>(fillQty) * static_cast<__int128>(level.price);

    Trade trade;

    trade.tradeId = nextTradeId_++;
    trade.orderId = order.orderId;
    trade.price = level.price;
    trade.quantity = fillQty;

    trade.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();

    report.trades.push_back(trade);

    spdlog::info(
        "[TRADE] "
        "orderId={} "
        "tradeId={} "
        "side=BUY "
        "price={} "
        "qty={}",

        trade.orderId, trade.tradeId,
        static_cast<double>(trade.price) / PRICE_SCALE,
        static_cast<double>(trade.quantity) / PRICE_SCALE);

    if (g_logger) {
      g_logger->logOrder(
          "[TRADE] orderId=" + std::to_string(trade.orderId) +
          " tradeId=" + std::to_string(trade.tradeId) + " side=BUY price=" +
          std::to_string(static_cast<double>(trade.price) / PRICE_SCALE) +
          " qty=" +
          std::to_string(static_cast<double>(trade.quantity) / PRICE_SCALE));
    }
  }

  report.filledQty = order.quantity - remaining;
  report.remainingQty = remaining;

  if (report.filledQty > 0) {
    report.averagePrice =
        static_cast<double>(totalCost / report.filledQty) / PRICE_SCALE;
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

  __int128 totalValue = 0;

  for (const auto& level : bids) {
    if (remaining <= 0) {
      break;
    }

    if (order.type == OrderType::Limit && level.price < order.price) {
      break;
    }

    int64_t fillQty = std::min(remaining, level.quantity);

    remaining -= fillQty;

    totalValue +=
        static_cast<__int128>(fillQty) * static_cast<__int128>(level.price);

    Trade trade;

    trade.tradeId = nextTradeId_++;
    trade.orderId = order.orderId;
    trade.price = level.price;
    trade.quantity = fillQty;

    trade.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();

    report.trades.push_back(trade);

    spdlog::info(
        "[TRADE] "
        "orderId={} "
        "tradeId={} "
        "side=SELL "
        "price={} "
        "qty={}",

        trade.orderId, trade.tradeId,
        static_cast<double>(trade.price) / PRICE_SCALE,
        static_cast<double>(trade.quantity) / PRICE_SCALE);

    if (g_logger) {
      g_logger->logOrder(
          "[TRADE] orderId=" + std::to_string(trade.orderId) +
          " tradeId=" + std::to_string(trade.tradeId) + " side=SELL price=" +
          std::to_string(static_cast<double>(trade.price) / PRICE_SCALE) +
          " qty=" +
          std::to_string(static_cast<double>(trade.quantity) / PRICE_SCALE));
    }
  }

  report.filledQty = order.quantity - remaining;
  report.remainingQty = remaining;

  if (report.filledQty > 0) {
    report.averagePrice =
        static_cast<double>(totalValue / report.filledQty) / PRICE_SCALE;
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

  if (g_logger) {
    g_logger->logOrder(
        "[OPEN_ORDER] orderId=" + std::to_string(order.orderId) +
        " remaining=" +
        std::to_string(static_cast<double>(report.remainingQty) / PRICE_SCALE));
  }
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

  if (g_logger) {
    g_logger->logOrder(
        "[OPEN_ORDER] orderId=" + std::to_string(order.orderId) +
        " remaining=" +
        std::to_string(static_cast<double>(report.remainingQty) / PRICE_SCALE));
  }

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

    spdlog::info(
        "[OPEN_ORDER_FILL] "
        "orderId={} "
        "filled={} "
        "remaining={}",

        orderId, report.filledQty, open.remainingQty);

    if (g_logger) {
      g_logger->logOrder(
          "[OPEN_ORDER_FILL] orderId=" + std::to_string(orderId) + " filled=" +
          std::to_string(static_cast<double>(report.filledQty) / PRICE_SCALE) +
          " remaining=" +
          std::to_string(static_cast<double>(open.remainingQty) / PRICE_SCALE));
    }

    if (open.remainingQty <= 0) {
      open.order.status = OrderStatus::Filled;

      completedOrders.push_back(orderId);

      spdlog::info(
          "[ORDER_COMPLETED] "
          "orderId={} "
          "totalFilled={}",

          orderId, open.order.filledQty);

      if (g_logger) {
        g_logger->logOrder(
            "[ORDER_COMPLETED] orderId=" + std::to_string(orderId) +
            " totalFilled=" +
            std::to_string(static_cast<double>(open.order.filledQty) /
                           PRICE_SCALE));
      }
      
    } else {
      open.order.status = OrderStatus::PartiallyFilled;
    }
  }

  for (auto orderId : completedOrders) {
    orderManager_.removeOrder(orderId);
  }
}