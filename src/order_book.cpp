#include "order_book.h"

#include <spdlog/spdlog.h>

#include <chrono>

OrderBook::OrderBook() : lastUpdateId(0) {}

void OrderBook::applyDelta(const DepthDelta& delta) {
  if (delta.finalUpdateId <= lastUpdateId) {
    return;
  }

  if (lastUpdateId != 0 && delta.firstUpdateId > lastUpdateId + 1) {
    spdlog::warn(
        "Missing updates detected "
        "lastUpdateId={} "
        "incoming U={}",
        lastUpdateId, delta.firstUpdateId);
  }

  auto update = [](auto& side, int64_t price, int64_t quantity) {
    if (quantity == 0) {
      side.erase(price);
    } else {
      side[price] = quantity;
    }
  };

  for (const auto& level : delta.bids) {
    update(bids, level.price, level.quantity);
  }

  for (const auto& level : delta.asks) {
    update(asks, level.price, level.quantity);
  }

  lastUpdateId = delta.finalUpdateId;
}

void OrderBook::setSnapshot(const BookSnapshot& snapshot) {
  bids.clear();
  asks.clear();

  for (const auto& level : snapshot.bids) {
    bids[level.price] = level.quantity;
  }

  for (const auto& level : snapshot.asks) {
    asks[level.price] = level.quantity;
  }

  lastUpdateId = snapshot.lastUpdateId;
}

BookSnapshot OrderBook::getSnapshot() const {
  BookSnapshot snapshot;

  snapshot.lastUpdateId = lastUpdateId;

  snapshot.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();

  snapshot.bids.reserve(bids.size());

  snapshot.asks.reserve(asks.size());

  for (const auto& [price, qty] : bids) {
    snapshot.bids.push_back({price, qty});
  }

  for (const auto& [price, qty] : asks) {
    snapshot.asks.push_back({price, qty});
  }

  return snapshot;
}

int64_t OrderBook::bestBid() const {
  if (bids.empty()) {
    return 0;
  }

  return bids.begin()->first;
}

int64_t OrderBook::bestAsk() const {
  if (asks.empty()) {
    return 0;
  }

  return asks.begin()->first;
}

int64_t OrderBook::spread() const {
  if (bids.empty() || asks.empty()) {
    return 0;
  }

  return bestAsk() - bestBid();
}

std::vector<PriceLevel> OrderBook::getBids(int levels) const {
  std::vector<PriceLevel> result;

  if (levels <= 0) {
    return result;
  }

  result.reserve(levels);

  for (auto it = bids.begin();
       it != bids.end() && static_cast<int>(result.size()) < levels; ++it) {
    result.push_back({it->first, it->second});
  }

  return result;
}

std::vector<PriceLevel> OrderBook::getAsks(int levels) const {
  std::vector<PriceLevel> result;

  if (levels <= 0) {
    return result;
  }

  result.reserve(levels);

  for (auto it = asks.begin();
       it != asks.end() && static_cast<int>(result.size()) < levels; ++it) {
    result.push_back({it->first, it->second});
  }

  return result;
}
