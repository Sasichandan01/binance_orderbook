#include <httplib.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <memory>
#include <nlohmann/json.hpp>
#include <thread>

#include "api_server.h"
#include "async_logger.h"
#include "execution_engine.h"
#include "order_book.h"
#include "order_manager.h"
#include "snapshot_manager.h"
#include "sync_engine.h"
#include "types.h"
#include "websocket_client.h"

using json = nlohmann::json;

namespace {
std::atomic<bool> running(true);

std::unique_ptr<AsyncLogger> g_logger;
}  // namespace

void signalHandler(int signal) {
  spdlog::info("Received signal {}, shutting down...", signal);

  running = false;

  if (g_logger) {
    g_logger->stop();
  }
}

static int64_t priceToInt(const std::string& s) {
  size_t dot = s.find('.');

  if (dot == std::string::npos) {
    return std::stoll(s) * PRICE_SCALE;
  }

  std::string intPart = s.substr(0, dot);
  std::string fracPart = s.substr(dot + 1);

  if (fracPart.length() > 8) {
    fracPart = fracPart.substr(0, 8);
  } else {
    fracPart.append(8 - fracPart.length(), '0');
  }

  return std::stoll(intPart) * PRICE_SCALE + std::stoll(fracPart);
}

bool fetchSnapshot(const std::string& symbol, BookSnapshot& snapshot,
                   int limit = 1000) {
  httplib::Client cli("https://api.binance.com");

  const std::string path =
      "/api/v3/depth?symbol=" + symbol + "&limit=" + std::to_string(limit);

  auto fetchStart = std::chrono::steady_clock::now();

  auto response = cli.Get(path);

  auto fetchEnd = std::chrono::steady_clock::now();

  auto fetchMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                     fetchEnd - fetchStart)
                     .count();

  spdlog::info(
      "[SNAPSHOT_FETCH] "
      "duration={}ms "
      "status={}",
      fetchMs, response ? response->status : -1);

  if (!response || response->status != 200) {
    spdlog::error("Failed to fetch snapshot");
    return false;
  }

  try {
    auto j = json::parse(response->body);

    snapshot.lastUpdateId = j["lastUpdateId"].get<uint64_t>();

    snapshot.timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();

    snapshot.bids.clear();
    snapshot.asks.clear();

    for (auto& bid : j["bids"]) {
      snapshot.bids.push_back(
          {.price = priceToInt(bid[0].get<std::string>()),
           .quantity = priceToInt(bid[1].get<std::string>())});
    }

    for (auto& ask : j["asks"]) {
      snapshot.asks.push_back(
          {.price = priceToInt(ask[0].get<std::string>()),
           .quantity = priceToInt(ask[1].get<std::string>())});
    }

    return true;
  } catch (const std::exception& e) {
    spdlog::error("Snapshot parse error: {}", e.what());
    return false;
  }
}

int main() {
  spdlog::set_level(spdlog::level::info);

  spdlog::info("Starting OrderBook Engine");

  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);

  g_logger = std::make_unique<AsyncLogger>("logs/orderbook.log");

  std::thread loggerThread([&]() { g_logger->run(); });

  OrderBook book;

  OrderManager orderManager;

  ExecutionEngine executionEngine(book, orderManager);

  ApiServer apiServer(executionEngine, orderManager);

  SnapshotManager snapshotManager("snapshots");

  SyncEngine syncEngine(book, [&](BookSnapshot& snapshot) {
    return fetchSnapshot("BTCUSDT", snapshot);
  });

  WebSocketClient websocket;

  websocket.connect("btcusdt", [&](const DepthDelta& delta) {
    syncEngine.processDelta(delta);
  });

  spdlog::info("Connected to Binance");

  spdlog::info("Buffering websocket events before snapshot fetch...");

  std::this_thread::sleep_for(std::chrono::seconds(2));

  if (!syncEngine.initialize()) {
    spdlog::error("Failed to initialize SyncEngine");

    g_logger->stop();

    if (loggerThread.joinable()) {
      loggerThread.join();
    }

    return 1;
  }

  snapshotManager.startPeriodicSaving(book, 30);

  std::thread apiThread([&]() { apiServer.start(8080); });

  spdlog::info("API thread started...");

  while (running) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  spdlog::info("Stopping...");

  websocket.disconnect();

  snapshotManager.stopPeriodicSaving();

  g_logger->stop();

  if (apiThread.joinable()) {
    apiThread.detach();
  }

  if (loggerThread.joinable()) {
    loggerThread.join();
  }

  if (g_logger && g_logger->droppedCount() > 0) {
    spdlog::warn("Dropped {} log entries", g_logger->droppedCount());
  }

  spdlog::info("Shutdown complete");

  return 0;
}