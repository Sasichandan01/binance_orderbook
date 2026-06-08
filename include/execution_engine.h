#ifndef EXECUTION_ENGINE_H
#define EXECUTION_ENGINE_H

#include "execution_report.h"
#include "order.h"
#include "order_book.h"
#include "order_manager.h"
#include "trade.h"

class ExecutionEngine {
 public:
  ExecutionEngine(OrderBook& book, OrderManager& orderManager);

  ExecutionReport placeOrder(Order order);

 private:
  OrderBook& book_;
  OrderManager& orderManager_;

  uint64_t nextTradeId_ = 1;

  ExecutionReport executeBuy(Order& order);

  ExecutionReport executeSell(Order& order);
};

#endif