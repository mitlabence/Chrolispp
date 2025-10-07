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

  void trace(const std::string& message); // technical log messages get a \t for easier readability
  void info(const std::string& message);   // general information messages
  void multiLineInfo(char* msg);          // for multi-line info messages
  void multiLineProtocol(char* msg);       // for multi-line protocol messages
  void error(const std::string& message);  // for critical errors
  void warning(
      const std::string& message);  // for warnings that are not critical errors
  void protocol(const std::string& message);  // for logging experiment protocol
                                              // steps (most relevant for the user)
  
 private:
  std::ofstream logFile;

  std::string getTimestamp();
};

#endif  // LOGGER_HPP
