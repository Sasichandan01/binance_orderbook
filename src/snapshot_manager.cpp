#include "snapshot_manager.h"

#include <spdlog/spdlog.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace fs = std::filesystem;

SnapshotManager::SnapshotManager(const std::string& dir)
    : snapshotDir(dir), running(false) {
  fs::create_directories(snapshotDir);
}

void SnapshotManager::saveSnapshot(const OrderBook& book) {
  const auto snapshot = book.getSnapshot();

  json j;

  j["lastUpdateId"] = snapshot.lastUpdateId;

  j["timestamp"] = snapshot.timestamp;

  j["bids"] = json::array();

  j["asks"] = json::array();

  for (const auto& level : snapshot.bids) {
    j["bids"].push_back({fromScaled(level.price), fromScaled(level.quantity)});
  }

  for (const auto& level : snapshot.asks) {
    j["asks"].push_back({fromScaled(level.price), fromScaled(level.quantity)});
  }

  const std::string filename =
      snapshotDir + "/snapshot_" + std::to_string(snapshot.timestamp) + ".json";

  std::ofstream file(filename);

  if (!file.is_open()) {
    spdlog::error("Failed to save snapshot {}", filename);

    return;
  }

  file << j.dump(2);

  spdlog::info("Snapshot saved {}", filename);
}

bool SnapshotManager::loadLatestSnapshot(OrderBook& book) {
  if (!fs::exists(snapshotDir)) {
    return false;
  }

  std::string latestFile;

  fs::file_time_type latestTime;

  for (const auto& entry : fs::directory_iterator(snapshotDir)) {
    if (entry.path().extension() != ".json") {
      continue;
    }

    auto fileTime = fs::last_write_time(entry);

    if (latestFile.empty() || fileTime > latestTime) {
      latestFile = entry.path().string();

      latestTime = fileTime;
    }
  }

  if (latestFile.empty()) {
    return false;
  }

  std::ifstream file(latestFile);

  if (!file.is_open()) {
    return false;
  }

  json j;

  file >> j;

  BookSnapshot snapshot;

  snapshot.lastUpdateId = j["lastUpdateId"].get<uint64_t>();

  snapshot.timestamp = j.value("timestamp", 0ULL);

  for (const auto& bid : j["bids"]) {
    snapshot.bids.push_back({toScaled(bid[0].get<double>()),

                             toScaled(bid[1].get<double>())});
  }

  for (const auto& ask : j["asks"]) {
    snapshot.asks.push_back({toScaled(ask[0].get<double>()),

                             toScaled(ask[1].get<double>())});
  }

  book.setSnapshot(snapshot);

  spdlog::info("Loaded snapshot {}", latestFile);

  return true;
}

void SnapshotManager::saveLoop(OrderBook* book, int interval) {
  while (running) {
    std::this_thread::sleep_for(std::chrono::seconds(interval));

    if (running) {
      saveSnapshot(*book);
    }
  }
}

void SnapshotManager::startPeriodicSaving(OrderBook& book,
                                          int intervalSeconds) {
  if (running) {
    return;
  }

  running = true;

  worker =
      std::thread(&SnapshotManager::saveLoop, this, &book, intervalSeconds);
}

void SnapshotManager::stopPeriodicSaving() {
  if (!running) {
    return;
  }

  running = false;

  if (worker.joinable()) {
    worker.join();
  }
}