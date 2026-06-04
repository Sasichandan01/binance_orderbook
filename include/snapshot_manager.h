#ifndef SNAPSHOT_MANAGER_H
#define SNAPSHOT_MANAGER_H

#include <atomic>
#include <string>
#include <thread>

#include "order_book.h"

class SnapshotManager {
 public:
  explicit SnapshotManager(const std::string& snapshotDir);

  void saveSnapshot(const OrderBook& book);

  bool loadLatestSnapshot(OrderBook& book);

  void startPeriodicSaving(OrderBook& book, int intervalSeconds);

  void stopPeriodicSaving();

 private:
  void saveLoop(OrderBook* book, int intervalSeconds);

 private:
  std::string snapshotDir;

  std::atomic<bool> running{false};

  std::thread worker;
};

#endif