#include <COMFunctions.hpp>
#include <stdexcept>

std::wstring getPortName(int portNumber) {
  if (portNumber < 0) {
    throw std::invalid_argument("Invalid port number: " +
                                std::to_string(portNumber));
  }
  std::wstring portName = (portNumber < 10)
                              ? L"COM" + std::to_wstring(portNumber)
                              : L"\\\\.\\COM" + std::to_wstring(portNumber);
  return portName;
}

HANDLE openSerialHandle(const std::wstring portName) {
  HANDLE h_Serial = CreateFileW(portName.c_str(), GENERIC_READ | GENERIC_WRITE,
                                0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (h_Serial == INVALID_HANDLE_VALUE) {
    if (GetLastError() == ERROR_FILE_NOT_FOUND) {
      throw com_init_error("Serial port does not exist");
    }
    throw com_init_error("Error opening serial port");
  }

  return h_Serial;
}

HANDLE createSerialHandle(const WCHAR* comPort) {
  HANDLE h_Serial = CreateFile(comPort, GENERIC_READ | GENERIC_WRITE, 0, 0,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (h_Serial == INVALID_HANDLE_VALUE) {
    if (GetLastError() == ERROR_FILE_NOT_FOUND) {
      throw com_init_error("Serial port does not exist");
    }
    throw com_init_error("Error opening serial port");
  }

  return h_Serial;
}

void configureSerialPort(HANDLE h_Serial) {
  DCB dcbSerialParam = {0};
  dcbSerialParam.DCBlength = sizeof(dcbSerialParam);
  if (!GetCommState(h_Serial, &dcbSerialParam)) {
    throw std::runtime_error("Error getting state");
  }

  dcbSerialParam.BaudRate = CBR_9600;
  dcbSerialParam.ByteSize = 8;
  dcbSerialParam.StopBits = ONESTOPBIT;
  dcbSerialParam.Parity = NOPARITY;

  if (!SetCommState(h_Serial, &dcbSerialParam)) {
    throw serial_port_config_error("Error setting state");
  }
}

WCHAR* stringToWCHAR(const std::string& str) {
  // Calculate the length of the wide character string
  int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);

  // Allocate memory for the wide character string
  WCHAR* wStr = new WCHAR[len];

  // Perform the conversion
  MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wStr, len);

  return wStr;
}


void configureTimeoutSettings(HANDLE h_Serial) {
  COMMTIMEOUTS timeout = {0};
  timeout.ReadIntervalTimeout = 60;
  timeout.ReadTotalTimeoutConstant = 60;
  timeout.ReadTotalTimeoutMultiplier = 15;
  timeout.WriteTotalTimeoutConstant = 60;
  timeout.WriteTotalTimeoutMultiplier = 8;
  if (!SetCommTimeouts(h_Serial, &timeout)) {
    throw timeout_setting_error("Error setting timeouts");
  }
}
void writeMessage(HANDLE h_Serial, const char* data, size_t dataSize) {
  DWORD dwBytesWritten;
  if (!WriteFile(h_Serial, data, dataSize, &dwBytesWritten, NULL)) {
    throw com_io_error("Error writing to serial port");
  }
}

char* readMessage(HANDLE h_Serial, size_t dataSize) {
  char* data = new char[dataSize];
  DWORD dwBytesRead;
  if (!ReadFile(h_Serial, data, dataSize, &dwBytesRead, NULL)) {
    delete[] data;
    throw com_io_error("Error reading from serial port");
  }
  return data;
}