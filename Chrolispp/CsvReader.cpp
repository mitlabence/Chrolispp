#include <string>
#include <shlobj.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "ProtocolStep.hpp"

std::string BrowseFile(std::string saved_path) {
  TCHAR szFile[MAX_PATH] = {0};

  OPENFILENAME ofn = {0};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = NULL;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = L"All Files\0*.*\0CSV Files\0*.csv\0";
  ofn.lpstrTitle = L"Choose a CSV file";
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if (GetOpenFileName(&ofn) == TRUE) {
    std::wstring ws(ofn.lpstrFile);
    std::string filePath(ws.begin(), ws.end());
    return filePath;
  }

  return "";
}

std::vector<ProtocolStep> readProtocolCSV(const std::string& filePath) {
  std::vector<ProtocolStep> protocolSteps;
  std::ifstream file(filePath);
  if (!file.is_open()) {
    std::cerr << "Error opening file: " << filePath << std::endl;
    return protocolSteps;
  }

  std::string line;
  while (std::getline(file, line)) {
    std::stringstream ss(line);
    std::vector<int> row;
    std::string value;
    int columnCount = 0;

    // Read the first five columns
    while (std::getline(ss, value, ',') && columnCount < 5) {
      try {
        int intValue = std::stoi(value);
        row.push_back(intValue);
      } catch (const std::invalid_argument& e) {
        std::cerr << "Invalid integer in CSV file: " << value << std::endl;
        continue;
      }
      columnCount++;
    }

    // Ensure there are exactly 5 values read
    if (row.size() == 5) {
      ProtocolStep step(row[0], row[1], row[2], row[3], row[4]);
      protocolSteps.push_back(step);
    }
  }

  file.close();
  return protocolSteps;
}


bool isCSVFile(const std::string& filePath) {
  // Check if the filePath ends with ".csv"
  if (filePath.length() >= 4 &&
      filePath.substr(filePath.length() - 4) == ".csv") {
    return true;
  }
  return false;
}