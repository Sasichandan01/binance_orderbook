#include "sync_engine.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>

SyncEngine::SyncEngine(OrderBook& book, SnapshotFetcher fetcher)
    : book_(book), fetcher_(std::move(fetcher)) {
  metrics_.start();
}

SyncEngine::~SyncEngine() { metrics_.stop(); }

bool SyncEngine::initialize() { return initializeFromSnapshot(); }

bool SyncEngine::isSynced() const { return synced_; }

bool SyncEngine::initializeFromSnapshot() {
  BookSnapshot snap;

  if (!fetcher_(snap)) {
    spdlog::error("Snapshot fetch failed");

    return false;
  }

  book_.setSnapshot(snap);

  expectedNextId_ = snap.lastUpdateId + 1;

  lastAppliedU_ = 0;

  synced_ = false;

  initialized_ = true;

  spdlog::info(
      "Snapshot initialized "
      "lastUpdateId={} "
      "expectedNextId={}",
      snap.lastUpdateId, expectedNextId_);

  return true;
}

void SyncEngine::processBufferedEvents() {
  bool started = false;

  for (const auto& delta : deltaBuffer_) {
    if (!started) {
      if (delta.firstUpdateId <= expectedNextId_ &&
          delta.finalUpdateId >= expectedNextId_) {
        started = true;
      } else {
        continue;
      }
    }

    auto bookStart = std::chrono::steady_clock::now();

    book_.applyDelta(delta);

    auto bookEnd = std::chrono::steady_clock::now();

    const auto applyLatencyUs =
        std::chrono::duration_cast<std::chrono::microseconds>(bookEnd -
                                                              bookStart)
            .count();

    spdlog::info(
        "[SYNC_REPLAY] "
        "u={} "
        "book_update={}us",
        delta.finalUpdateId, applyLatencyUs);

    metrics_.record(applyLatencyUs, calculateExchangeLatencyMs(delta));

    lastAppliedU_ = delta.finalUpdateId;
  }

  synced_ = true;

  expectedNextId_ = lastAppliedU_ + 1;

  deltaBuffer_.clear();

  spdlog::info(
      "Synchronization complete "
      "lastAppliedU={}",
      lastAppliedU_);
}

int64_t SyncEngine::calculateExchangeLatencyMs(const DepthDelta& delta) const {
  if (delta.eventTime == 0) {
    return -1;
  }

  const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();

  return nowMs - delta.eventTime;
}

void SyncEngine::processDelta(const DepthDelta& delta) {
  if (!initialized_) {
    deltaBuffer_.push_back(delta);
    return;
  }

  auto eventStart = std::chrono::steady_clock::now();

  auto exchangeToAppMs =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count() -
      delta.eventTime;

  if (!synced_) {
    deltaBuffer_.push_back(delta);

    if (expectedNextId_ > 0) {
      const uint64_t lastUpdateId = expectedNextId_ - 1;

      deltaBuffer_.erase(
          std::remove_if(deltaBuffer_.begin(), deltaBuffer_.end(),
                         [&](const DepthDelta& d) {
                           return d.finalUpdateId < lastUpdateId;
                         }),
          deltaBuffer_.end());
    }

    for (const auto& d : deltaBuffer_) {
      if (d.firstUpdateId <= expectedNextId_ &&
          d.finalUpdateId >= expectedNextId_) {
        auto syncStart = std::chrono::steady_clock::now();

        processBufferedEvents();

        auto syncEnd = std::chrono::steady_clock::now();

        auto syncUs = std::chrono::duration_cast<std::chrono::microseconds>(
                          syncEnd - syncStart)
                          .count();

        spdlog::info(
            "[SYNC_COMPLETE] "
            "expectedNextId={} "
            "buffered_events={} "
            "sync_processing={}us",
            expectedNextId_, deltaBuffer_.size(), syncUs);

        return;
      }
    }

    if (deltaBuffer_.size() > 2000) {
      spdlog::warn(
          "Sync buffer overflow. "
          "Resynchronizing.");

      initializeFromSnapshot();
    }

    return;
  }

  if (lastAppliedU_ != 0) {
    if (delta.firstUpdateId > lastAppliedU_ + 1) {
      spdlog::warn(
          "Gap detected. "
          "Expected={} "
          "Received={}",
          lastAppliedU_ + 1, delta.firstUpdateId);

      initializeFromSnapshot();

      deltaBuffer_.push_back(delta);

      return;
    }
  }

  auto start = std::chrono::steady_clock::now();

  book_.applyDelta(delta);

  auto end = std::chrono::steady_clock::now();

  const auto applyLatencyUs =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();

  metrics_.record(applyLatencyUs, calculateExchangeLatencyMs(delta));

  auto totalUs = std::chrono::duration_cast<std::chrono::microseconds>(
                     std::chrono::steady_clock::now() - eventStart)
                     .count();

  spdlog::info(
      "[LATENCY_BREAKDOWN] "
      "u={} "
      "exchange_to_app={}ms "
      "book_update={}us "
      "total_processing={}us",
      delta.finalUpdateId, exchangeToAppMs, applyLatencyUs, totalUs);

  lastAppliedU_ = delta.finalUpdateId;

  const auto bestBid = book_.bestBid();

  const auto bestAsk = book_.bestAsk();

  if (bestBid > 0 && bestAsk > 0 && bestBid >= bestAsk) {
    spdlog::error(
        "Crossed book detected "
        "bid={} ask={}",
        bestBid, bestAsk);

    initializeFromSnapshot();
  }
}