#include "websocket_client.h"

#include <httplib.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <nlohmann/json.hpp>
#include <thread>

using json = nlohmann::json;

namespace {
int64_t priceToInt(const std::string& value) {
  size_t dot = value.find('.');

  if (dot == std::string::npos) {
    return std::stoll(value) * PRICE_SCALE;
  }

  std::string intPart = value.substr(0, dot);

  std::string fracPart = value.substr(dot + 1);

  if (fracPart.size() > 8) {
    fracPart = fracPart.substr(0, 8);
  } else {
    fracPart.append(8 - fracPart.size(), '0');
  }

  return std::stoll(intPart) * PRICE_SCALE + std::stoll(fracPart);
}
}  // namespace

class WebSocketClient::Impl {
 public:
  void connect(const std::string& symbol, DepthCallback callback) {
    callback_ = std::move(callback);

    running_ = true;

    worker_ = std::thread([this, symbol]() {
      const std::string url =
          "wss://stream.binance.com:9443/ws/" + symbol + "@depth";

      httplib::ws::WebSocketClient ws(url);

      if (!ws.is_valid()) {
        spdlog::error("Invalid websocket URL");

        return;
      }

      while (running_) {
        if (!ws.connect()) {
          spdlog::warn("Connection failed. Retrying...");

          std::this_thread::sleep_for(std::chrono::seconds(3));

          continue;
        }

        spdlog::info("Websocket connected");

        while (running_ && ws.is_open()) {
          std::string message;

          auto result = ws.read(message);

          if (result == httplib::ws::Fail) {
            spdlog::warn("Websocket disconnected");

            break;
          }

          try {
            auto j = json::parse(message);

            if (!j.contains("U") || !j.contains("u")) {
              continue;
            }

            DepthDelta delta;

            delta.firstUpdateId = j["U"].get<uint64_t>();

            delta.finalUpdateId = j["u"].get<uint64_t>();

            delta.eventTime = j["E"].get<uint64_t>();

            for (auto& bid : j["b"]) {
              delta.bids.push_back({priceToInt(bid[0].get<std::string>()),

                                    priceToInt(bid[1].get<std::string>())});
            }

            for (auto& ask : j["a"]) {
              delta.asks.push_back({priceToInt(ask[0].get<std::string>()),

                                    priceToInt(ask[1].get<std::string>())});
            }

            if (callback_) {
              callback_(delta);
            }
          } catch (const std::exception& e) {
            spdlog::warn("JSON parse error: {}", e.what());
          }
        }

        if (running_) {
          std::this_thread::sleep_for(std::chrono::seconds(3));
        }
      }
    });
  }

  void disconnect() {
    running_ = false;

    if (worker_.joinable()) {
      worker_.join();
    }
  }

  bool isConnected() const { return running_; }

 private:
  std::atomic<bool> running_{false};

  std::thread worker_;

  DepthCallback callback_;
};

WebSocketClient::WebSocketClient() : pImpl(new Impl()) {}

WebSocketClient::~WebSocketClient() { delete pImpl; }

void WebSocketClient::connect(const std::string& symbol,
                              DepthCallback callback) {
  pImpl->connect(symbol, std::move(callback));
}

void WebSocketClient::disconnect() { pImpl->disconnect(); }

bool WebSocketClient::isConnected() const { return pImpl->isConnected(); }