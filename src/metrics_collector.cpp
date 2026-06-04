#include "metrics_collector.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <numeric>

MetricsCollector::MetricsCollector() {}

MetricsCollector::~MetricsCollector() { stop(); }

void MetricsCollector::start() {
  if (running_) return;

  running_ = true;

  worker_ = std::thread(&MetricsCollector::workerLoop, this);
}

void MetricsCollector::stop() {
  running_ = false;

  if (worker_.joinable()) {
    worker_.join();
  }
}

void MetricsCollector::record(int64_t applyLatencyUs,
                              int64_t exchangeLatencyMs) {
  std::lock_guard<std::mutex> lock(mutex_);

  applyLatencies_.push_back(applyLatencyUs);

  if (exchangeLatencyMs >= 0) {
    exchangeLatencies_.push_back(exchangeLatencyMs);
  }
}

MetricsSnapshot MetricsCollector::getSnapshot() const {
  std::lock_guard<std::mutex> lock(mutex_);

  return latestSnapshot_;
}

void MetricsCollector::workerLoop() {
  while (running_) {
    std::this_thread::sleep_for(std::chrono::seconds(REPORT_INTERVAL_SEC));

    std::vector<int64_t> applyCopy;
    std::vector<int64_t> exchangeCopy;

    {
      std::lock_guard<std::mutex> lock(mutex_);

      applyCopy.swap(applyLatencies_);

      exchangeCopy.swap(exchangeLatencies_);
    }

    if (applyCopy.empty()) {
      continue;
    }

    std::sort(applyCopy.begin(), applyCopy.end());

    MetricsSnapshot snapshot;

    snapshot.count = applyCopy.size();

    snapshot.min_us = applyCopy.front();

    snapshot.max_us = applyCopy.back();

    snapshot.avg_us = static_cast<double>(std::accumulate(
                          applyCopy.begin(), applyCopy.end(), int64_t{0})) /
                      applyCopy.size();

    snapshot.p50_us = applyCopy[applyCopy.size() * 50 / 100];

    snapshot.p95_us = applyCopy[applyCopy.size() * 95 / 100];

    snapshot.p99_us =
        applyCopy[std::min(applyCopy.size() - 1, applyCopy.size() * 99 / 100)];

    if (!exchangeCopy.empty()) {
      std::sort(exchangeCopy.begin(), exchangeCopy.end());

      snapshot.exchange_latency_avg_ms =
          static_cast<double>(std::accumulate(exchangeCopy.begin(),
                                              exchangeCopy.end(), int64_t{0})) /
          exchangeCopy.size();

      snapshot.exchange_latency_p95_ms =
          exchangeCopy[exchangeCopy.size() * 95 / 100];
    }

    {
      std::lock_guard<std::mutex> lock(mutex_);

      latestSnapshot_ = snapshot;
    }

    publishMetrics(snapshot);
  }
}

void MetricsCollector::publishMetrics(const MetricsSnapshot& m) {
  spdlog::info(
      "[METRICS] "
      "count={} "
      "avg={}us "
      "min={}us "
      "max={}us "
      "p50={}us "
      "p95={}us "
      "p99={}us "
      "exchange_avg={}ms "
      "exchange_p95={}ms",

      m.count,

      m.avg_us,

      m.min_us, m.max_us,

      m.p50_us, m.p95_us, m.p99_us,

      m.exchange_latency_avg_ms, m.exchange_latency_p95_ms);
}