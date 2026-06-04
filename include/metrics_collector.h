#ifndef METRICS_COLLECTOR_H
#define METRICS_COLLECTOR_H

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

struct MetricsSnapshot {
  double avg_us = 0.0;

  int64_t min_us = 0;
  int64_t max_us = 0;

  int64_t p50_us = 0;
  int64_t p95_us = 0;
  int64_t p99_us = 0;

  uint64_t count = 0;

  double exchange_latency_avg_ms = 0.0;
  int64_t exchange_latency_p95_ms = 0;
};

class MetricsCollector {
 public:
  MetricsCollector();
  ~MetricsCollector();

  void start();
  void stop();

  void record(int64_t applyLatencyUs, int64_t exchangeLatencyMs = -1);

  MetricsSnapshot getSnapshot() const;

 private:
  void workerLoop();
  void publishMetrics(const MetricsSnapshot& snapshot);

 private:
  mutable std::mutex mutex_;

  std::vector<int64_t> applyLatencies_;
  std::vector<int64_t> exchangeLatencies_;

  std::thread worker_;

  std::atomic<bool> running_{false};

  MetricsSnapshot latestSnapshot_;

  static constexpr uint64_t REPORT_INTERVAL_SEC = 30;
};

#endif