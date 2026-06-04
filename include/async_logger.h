#ifndef ASYNC_LOGGER_H
#define ASYNC_LOGGER_H

#include <atomic>
#include <fstream>
#include <vector>

#include "types.h"

class AsyncLogger {
 private:
  struct LogEntry {
    uint64_t timestamp_ns;

    uint64_t update_id;

    bool isBid;

    int64_t price;

    int64_t quantity;
  };

  static constexpr size_t QUEUE_SIZE = 100000;

  std::vector<LogEntry> queue_;

  std::atomic<size_t> head_{0};
  std::atomic<size_t> tail_{0};

  std::atomic<bool> running_{true};

  std::atomic<size_t> dropped_{0};

  std::ofstream file_;

 public:
  explicit AsyncLogger(const std::string& filename);

  ~AsyncLogger();

  void logRawDelta(uint64_t timestampNs, const DepthDelta& delta);

  void run();

  void stop();

  bool isOpen() const { return file_.is_open(); }

  size_t droppedCount() const { return dropped_.load(); }
};

#endif