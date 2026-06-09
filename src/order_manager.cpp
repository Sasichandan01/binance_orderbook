#include "order_manager.h"

uint64_t OrderManager::addOrder(const Order& order) {
  OpenOrder open;

  open.order = order;

  open.remainingQty = order.quantity - order.filledQty;

  openOrders_[order.orderId] = open;

  return order.orderId;
}

bool OrderManager::cancelOrder(uint64_t orderId) {
  auto it = openOrders_.find(orderId);

  if (it == openOrders_.end()) {
    return false;
  }

  it->second.order.status = OrderStatus::Cancelled;

  openOrders_.erase(it);

  return true;
}

OpenOrder* OrderManager::findOrder(uint64_t orderId) {
  auto it = openOrders_.find(orderId);

  if (it == openOrders_.end()) {
    return nullptr;
  }

  return &it->second;
}

std::vector<OpenOrder> OrderManager::getAllOrders() const {
  std::vector<OpenOrder> result;

  result.reserve(openOrders_.size());

  for (const auto& [id, order] : openOrders_) {
    result.push_back(order);
  }

  return result;
}

bool OrderManager::removeOrder(uint64_t orderId) {
  return openOrders_.erase(orderId) > 0;
}

uint64_t OrderManager::generateOrderId() { return nextOrderId_++; }

std::unordered_map<uint64_t, OpenOrder>& OrderManager::getOpenOrders() {
  return openOrders_;
}