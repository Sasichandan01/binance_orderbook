#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include <cstdint>
#include <map>
#include <vector>

#include "types.h"

class OrderBook {
 public:
  OrderBook();

  void applyDelta(const DepthDelta& delta);

  void setSnapshot(const BookSnapshot& snapshot);

  BookSnapshot getSnapshot() const;

  int64_t bestBid() const;

  int64_t bestAsk() const;

  int64_t spread() const;

  std::vector<PriceLevel> getBids(int levels) const;

  std::vector<PriceLevel> getAsks(int levels) const;

  uint64_t getLastUpdateId() const { return lastUpdateId; }

  double getBestBid() const {
    return static_cast<double>(bestBid()) / PRICE_SCALE;
  }

  double getBestAsk() const {
    return static_cast<double>(bestAsk()) / PRICE_SCALE;
  }

 private:
  std::map<int64_t, int64_t, std::greater<int64_t>> bids;

  std::map<int64_t, int64_t> asks;

  uint64_t lastUpdateId = 0;
};

#endif