#include "async_logger.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <thread>

AsyncLogger::AsyncLogger(const std::string& filename) : queue_(QUEUE_SIZE) {
  file_.open(filename, std::ios::out | std::ios::trunc);

  if (!file_) {
    spdlog::warn("Could not open log file '{}'", filename);

    return;
  }

  file_ << "timestamp_ns,"
        << "update_id,"
        << "side,"
        << "price,"
        << "quantity\n";

  file_.flush();

  spdlog::info("AsyncLogger initialized: {}", filename);
}

AsyncLogger::~AsyncLogger() { stop(); }

void AsyncLogger::logRawDelta(uint64_t timestampNs, const DepthDelta& delta) {
  if (!file_.is_open()) {
    return;
  }

  auto push = [&](bool isBid, int64_t price, int64_t quantity) {
    size_t head = head_.load(std::memory_order_relaxed);

    size_t nextHead = (head + 1) % QUEUE_SIZE;

    if (nextHead == tail_.load(std::memory_order_acquire)) {
      dropped_.fetch_add(1, std::memory_order_relaxed);

      return;
    }

    auto& entry = queue_[head];

    entry.timestamp_ns = timestampNs;

    entry.update_id = delta.finalUpdateId;

    entry.isBid = isBid;

    entry.price = price;

    entry.quantity = quantity;

    head_.store(nextHead, std::memory_order_release);
  };

  for (const auto& bid : delta.bids) {
    push(true, bid.price, bid.quantity);
  }

  for (const auto& ask : delta.asks) {
    push(false, ask.price, ask.quantity);
  }
}

void AsyncLogger::run() {
  spdlog::info("AsyncLogger worker started");

  while (running_) {
    size_t tail = tail_.load(std::memory_order_relaxed);

    size_t head = head_.load(std::memory_order_acquire);

    while (tail != head) {
      const auto& entry = queue_[tail];

      file_ << entry.timestamp_ns << "," << entry.update_id << ","
            << (entry.isBid ? "bid" : "ask") << "," << entry.price << ","
            << entry.quantity << "\n";

      tail = (tail + 1) % QUEUE_SIZE;
    }

    tail_.store(tail, std::memory_order_release);

    static uint64_t flushCounter = 0;

    if (++flushCounter % 100 == 0) {
      file_.flush();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Final drain

  size_t tail = tail_.load(std::memory_order_relaxed);

  size_t head = head_.load(std::memory_order_acquire);

  while (tail != head) {
    const auto& entry = queue_[tail];

    file_ << entry.timestamp_ns << "," << entry.update_id << ","
          << (entry.isBid ? "bid" : "ask") << "," << entry.price << ","
          << entry.quantity << "\n";

    tail = (tail + 1) % QUEUE_SIZE;
  }

  tail_.store(tail, std::memory_order_release);

  if (file_.is_open()) {
    file_.flush();
    file_.close();
  }

  spdlog::info("AsyncLogger worker stopped");
}

void AsyncLogger::stop() { running_ = false; }