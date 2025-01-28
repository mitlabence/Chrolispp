#include "Logger.hpp"

Logger::Logger(const std::string& filename) {
  logFile.open(filename, std::ios::out | std::ios::app);
  if (!logFile.is_open()) {
    throw std::runtime_error("Could not open log file: " + filename);
  }
}

Logger::~Logger() {
  if (logFile.is_open()) {
    logFile.close();
  }
}

void Logger::log(const std::string& message) {
  logFile << getTimestamp() << " - " << message << std::endl;
}

void Logger::error(const std::string& message) {
  logFile << getTimestamp() << " ERROR: " << message << std::endl;
}
void Logger::info(const std::string& message) {
  logFile << getTimestamp() << " INFO: " << message << std::endl;
}

void Logger::warning(const std::string& message) {
  logFile << getTimestamp() << " WARNING: " << message << std::endl;
}

std::string Logger::getTimestamp() {
  auto now = std::chrono::system_clock::now();
  std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm now_tm;
  localtime_s(&now_tm, &now_time);

  char buffer[30];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &now_tm);

  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;
  char timestamp[40];
  std::snprintf(timestamp, sizeof(timestamp), "%s.%03d", buffer,
                static_cast<int>(ms.count()));

  return timestamp;
}
