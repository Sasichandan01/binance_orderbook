#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <functional>
#include <string>

#include "types.h"

using DepthCallback = std::function<void(const DepthDelta&)>;

class WebSocketClient {
 public:
  WebSocketClient();

  ~WebSocketClient();

  void connect(const std::string& symbol, DepthCallback callback);

  void disconnect();

  bool isConnected() const;

 private:
  class Impl;

  Impl* pImpl;
};

#endif