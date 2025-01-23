#ifndef CSV_READER_HPP
#define CSV_READER_HPP

#include <string>
#include <shlobj.h>
#include <sstream>
#include <vector>
#include "ProtocolStep.hpp"

// BrowseFolder function declaration
std::string BrowseFile(std::string saved_path);

// readFirstFiveColumns function declaration
std::vector<ProtocolStep> readProtocolCSV(const std::string& filePath);

bool isCSVFile(const std::string& filePath);

#endif  // CSV_READER_HPP
