#ifndef API_SERVER_H
#define API_SERVER_H

#include "execution_engine.h"
#include "httplib.h"
#include "order_manager.h"

class ApiServer {
 public:
  ApiServer(ExecutionEngine& engine, OrderManager& orderManager);

  void start(int port);

 private:
  ExecutionEngine& executionEngine_;
  OrderManager& orderManager_;
};

#endif