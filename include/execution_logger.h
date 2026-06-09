#ifndef EXECUTION_LOGGER_H
#define EXECUTION_LOGGER_H

#include <spdlog/logger.h>

#include <memory>

class ExecutionLogger {
 public:
  static void initialize();

  static std::shared_ptr<spdlog::logger> get();

 private:
  static std::shared_ptr<spdlog::logger> logger_;
};

#endif