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
#endif  // COM_FUNCTIONS_HPP