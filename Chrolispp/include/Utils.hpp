#ifndef UTILS_HPP
#define UTILS_HPP

#include <shlobj.h>

#include <sstream>
#include <string>
#include <vector>

#include "ProtocolStep.hpp"

std::string BrowseCSV();

std::string SelectFolderAndSuggestFile(const std::string& suggested_fname);

std::vector<ProtocolStep> readProtocolCSV(const std::string& filePath);

bool isCSVFile(const std::string& filePath);

std::string getCurrentDateTime();

std::string generateLogFileName(const std::string& prefix);

void showOpenCSVInstructions();

void intToCharArray(ViUInt16 number, char* charArray, size_t bufferSize);

#endif  // UTILS_HPP
