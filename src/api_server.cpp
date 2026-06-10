#include "api_server.h"

#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>

#include "types.h"

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

        order.price = toScaled(body.value("price", 0.0));

        order.quantity = toScaled(body["quantity"].get<double>());

        auto report = executionEngine_.placeOrder(order);

        json response;

        response["orderId"] = report.orderId;

        response["filledQty"] = fromScaled(report.filledQty);

        response["remainingQty"] = fromScaled(report.remainingQty);

        response["averagePrice"] = report.averagePrice;

        switch (report.status) {
          case OrderStatus::Filled:
            response["status"] = "FILLED";
            break;

          case OrderStatus::PartiallyFilled:
            response["status"] = "PARTIALLY_FILLED";
            break;

          case OrderStatus::Open:
            response["status"] = "OPEN";
            break;

          case OrderStatus::Cancelled:
            response["status"] = "CANCELLED";
            break;

          default:
            response["status"] = "REJECTED";
            break;
        }

        json trades = json::array();

        for (const auto& trade : report.trades) {
          json item;

          item["tradeId"] = trade.tradeId;

          item["price"] = fromScaled(trade.price);

          item["quantity"] = fromScaled(trade.quantity);

          item["timestamp"] = trade.timestamp;

          trades.push_back(item);
        }

        response["trades"] = trades;

        res.set_content(response.dump(2), "application/json");
      });

  server.Get("/orders", [&](const httplib::Request&, httplib::Response& res) {
    auto orders = orderManager_.getAllOrders();

    json result = json::array();

    for (const auto& open : orders) {
      json item;

      item["orderId"] = open.order.orderId;

      item["price"] = fromScaled(open.order.price);

      item["quantity"] = fromScaled(open.order.quantity);

      item["filledQty"] = fromScaled(open.order.filledQty);

      item["remainingQty"] = fromScaled(open.remainingQty);

      result.push_back(item);
    }

    res.set_content(result.dump(2), "application/json");
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