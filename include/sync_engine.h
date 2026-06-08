#ifndef SYNC_ENGINE_H
#define SYNC_ENGINE_H

#include <cstdint>
#include <functional>
#include <vector>

#include "metrics_collector.h"
#include "order_book.h"
#include "types.h"

class SyncEngine {
 public:
  using SnapshotFetcher = std::function<bool(BookSnapshot&)>;

  SyncEngine(OrderBook& book, SnapshotFetcher fetcher);

  ~SyncEngine();

  bool initialize();

  void processDelta(const DepthDelta& delta);

  bool isSynced() const;

 private:
  bool initializeFromSnapshot();

  void processBufferedEvents();

  int64_t calculateExchangeLatencyMs(const DepthDelta& delta) const;

 private:
  OrderBook& book_;

  SnapshotFetcher fetcher_;

  MetricsCollector metrics_;

  std::vector<DepthDelta> deltaBuffer_;

  bool synced_ = false;

  bool initialized_ = false;

  uint64_t expectedNextId_ = 0;

  uint64_t lastAppliedU_ = 0;
};

#endif