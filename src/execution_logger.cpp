#include "execution_logger.h"

#include <spdlog/sinks/basic_file_sink.h>

std::shared_ptr<spdlog::logger> ExecutionLogger::logger_ = nullptr;

void ExecutionLogger::initialize() {
  if (logger_) {
    return;
  }

  logger_ = spdlog::basic_logger_mt("executions", "logs/executions.log");

  logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
}

std::shared_ptr<spdlog::logger> ExecutionLogger::get() { return logger_; }