#ifndef COM_FUNCTIONS_HPP
#define COM_FUNCTIONS_HPP
#include <Windows.h>
#include <string>

HANDLE createSerialHandle(const WCHAR* comPort);
void configureSerialPort(HANDLE h_Serial);
WCHAR* stringToWCHAR(const std::string& str);
void configureTimeoutSettings(HANDLE h_Serial);
void writeMessage(HANDLE h_Serial, const char* data, size_t dataSize);
char* readMessage(HANDLE h_Serial, size_t dataSize);


class serial_port_config_error : public std::exception {
 public:
  explicit serial_port_config_error(const std::string& message)
      : message_(message) {}

  virtual const char* what() const noexcept override {
    return message_.c_str();
  }

 private:
  std::string message_;
};

class timeout_setting_error : public std::exception {
 public:
  explicit timeout_setting_error(const std::string& message)
      : message_(message) {}

  virtual const char* what() const noexcept override {
    return message_.c_str();
  }

 private:
  std::string message_;
};

class com_io_error : public std::exception {
 public:
  explicit com_io_error(const std::string& message)
      : message_(message) {}

  virtual const char* what() const noexcept override {
    return message_.c_str();
  }

 private:
  std::string message_;
};
class com_init_error : public std::exception {
 public:
  explicit com_init_error(const std::string& message) : message_(message) {}

  virtual const char* what() const noexcept override {
    return message_.c_str();
  }

 private:
  std::string message_;
};

#endif  // COM_FUNCTIONS_HPP