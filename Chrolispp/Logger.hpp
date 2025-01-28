#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <chrono>
#include <ctime>
#include <fstream>
#include <string>

class Logger {
 public:
  explicit Logger(const std::string& filename);
  ~Logger();

  void log(const std::string& message);
  void info(const std::string& message);
  void error(const std::string& message);
  void warning(const std::string& message);

 private:
  std::ofstream logFile;

  std::string getTimestamp();
};

#endif  // LOGGER_HPP
