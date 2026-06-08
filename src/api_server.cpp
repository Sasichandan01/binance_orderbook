#include "api_server.h"

#include <nlohmann/json.hpp>

#include <spdlog/spdlog.h>

using json = nlohmann::json;

ApiServer::ApiServer(ExecutionEngine& executionEngine,
                     OrderManager& orderManager)
    : executionEngine_(executionEngine), orderManager_(orderManager) {}

void ApiServer::start(int port) {
  httplib::Server server;

  server.Post(
      "/orders", [&](const httplib::Request& req, httplib::Response& res) {
        auto body = json::parse(req.body);

        Order order;

        order.orderId = orderManager_.generateOrderId();

        std::string side = body["side"].get<std::string>();

        std::string type = body["type"].get<std::string>();

        order.side = (side == "BUY") ? Side::Buy : Side::Sell;

        order.type = (type == "MARKET") ? OrderType::Market : OrderType::Limit;

        order.price = body.value("price", 0LL);

        order.quantity = body["quantity"].get<int64_t>();

        auto report = executionEngine_.placeOrder(order);

        json response;

        response["orderId"] = report.orderId;

        response["filledQty"] = report.filledQty;

        response["remainingQty"] = report.remainingQty;

        res.set_content(response.dump(), "application/json");
      });

  server.Get("/orders", [&](const httplib::Request&, httplib::Response& res) {
    auto orders = orderManager_.getAllOrders();

    json result = json::array();

    for (const auto& open : orders) {
      json item;

      item["orderId"] = open.order.orderId;

      item["remainingQty"] = open.remainingQty;

      result.push_back(item);
    }

    res.set_content(result.dump(), "application/json");
  });

  server.Delete(R"(/orders/(\d+))",
                [&](const httplib::Request& req, httplib::Response& res) {
                  uint64_t id = std::stoull(req.matches[1]);

                  bool success = orderManager_.cancelOrder(id);

                  json response;

                  response["success"] = success;

                  res.set_content(response.dump(), "application/json");
                });

  spdlog::info("[API] Listening on port {}", port);

  server.listen("0.0.0.0", port);
}