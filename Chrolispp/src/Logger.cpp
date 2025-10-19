#include "Logger.hpp"
// TODO: I think right now, all access is synchronous, so no need for mutexes.
// But if ever the possibility of async logging arises, need to implement a
// parallel-compatible (concurrent) queue and a separate writing
Logger::Logger(const std::string& filename) {
  logFile.open(filename, std::ios::out | std::ios::app);
  if (!logFile.is_open()) {
    throw std::runtime_error("Could not open log file: " + filename);
  }
}

Logger::~Logger() {
  flush();  // ensure all buffered messages are written
  if (logFile.is_open()) {
    logFile.close();
  }
}

void Logger::log(LogType type, const std::string& message) {
  buffer.push_back({type, getTimestamp(), message});
}

void Logger::trace(const std::string& message) { log(LogType::Trace, message); }
void Logger::error(const std::string& message) { log(LogType::Error, message); }
void Logger::info(const std::string& message) { log(LogType::Info, message); }
void Logger::protocol(const std::string& message) {
  log(LogType::Protocol, message);
}

void Logger::warning(const std::string& message) {
  log(LogType::Warning, message);
}

void Logger::multiLineInfo(char* msg) {
  std::istringstream stream(msg);
  std::string line;

  while (std::getline(stream, line)) {
    info(line);  // Each line gets its own timestamp
  }
}

void Logger::multiLineProtocol(char* msg) {
  std::istringstream stream(msg);
  std::string line;

  while (std::getline(stream, line)) {
    protocol(line);  // Each line gets its own timestamp
  }
}

std::string Logger::format(const LogMessage& msg) {
  std::string typeStr;
  switch (msg.type) {
    case LogType::Trace:
      typeStr = "[TRACE]";
      break;
    case LogType::Error:
      typeStr = "[ERROR]";
      break;
    case LogType::Info:
      typeStr = "[INFO]";
      break;
    case LogType::Protocol:
      typeStr = "[PROTOCOL]";
      break;
    case LogType::Warning:
      typeStr = "[WARNING]";
      break;
  }
  std::ostringstream oss;
  oss << msg.timestamp << "\t" << typeStr << "\t" << msg.message;
  return oss.str();
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

void Logger::flush() {
  for (const auto& msg : buffer) {
    logFile << format(msg) << std::endl;
  }
  buffer.clear();
}