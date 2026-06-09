#ifndef ORDER_MANAGER_H
#define ORDER_MANAGER_H

#include <unordered_map>
#include <vector>

#include "open_order.h"

class OrderManager {
 public:
  uint64_t addOrder(const Order& order);

  bool cancelOrder(uint64_t orderId);

  OpenOrder* findOrder(uint64_t orderId);

  std::vector<OpenOrder> getAllOrders() const;

  bool removeOrder(uint64_t orderId);

  uint64_t generateOrderId();

  std::unordered_map<uint64_t, OpenOrder>& getOpenOrders();

 private:
  uint64_t nextOrderId_ = 1;

  std::unordered_map<uint64_t, OpenOrder> openOrders_;
};

#endif