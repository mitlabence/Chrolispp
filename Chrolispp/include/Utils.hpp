#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <shlobj.h>
#include <sstream>
#include <vector>
#include "ProtocolStep.hpp"

std::string BrowseCSV();

std::string SelectFolderAndSuggestFile(const std::string& suggested_fname);

std::vector<ProtocolStep> readProtocolCSV(const std::string& filePath);

bool isCSVFile(const std::string& filePath);

std::string getCurrentDateTime();

std::string generateLogFileName(const std::string& prefix);


void showOpenCSVInstructions();
#endif  // UTILS_HPP
