#ifndef ASYNC_LOGGER_H
#define ASYNC_LOGGER_H

#include <atomic>
#include <fstream>
#include <string>
#include <vector>

#include "types.h"

class AsyncLogger {
 private:
  enum class EntryType { RawDelta, Message };

  struct LogEntry {
    EntryType type = EntryType::Message;

    bool isOrderEvent = false;

    uint64_t timestampNs = 0;

    uint64_t updateId = 0;

    bool isBid = false;

    int64_t price = 0;

    int64_t quantity = 0;

    std::string message;
  };

  static constexpr size_t QUEUE_SIZE = 100000;

  std::vector<LogEntry> queue_;

  std::atomic<size_t> head_{0};

  std::atomic<size_t> tail_{0};

  std::atomic<bool> running_{true};

  std::atomic<size_t> dropped_{0};

  std::ofstream file_;

  std::ofstream ordersFile_;

 public:
  explicit AsyncLogger(const std::string& filename);

  ~AsyncLogger();

  void logRawDelta(uint64_t timestampNs, const DepthDelta& delta);

  void logMessage(const std::string& message);

  void run();

  void stop();

  bool isOpen() const { return file_.is_open(); }

  size_t droppedCount() const { return dropped_.load(); }

  void logOrder(const std::string& message);
};

extern std::unique_ptr<AsyncLogger> g_logger;

#endif