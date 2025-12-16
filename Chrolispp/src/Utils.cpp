#include <shlobj.h>
#include <shobjidl.h>
#include <windows.h>

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "ProtocolStep.hpp"
#include "Utils.hpp"

std::string BrowseCSV() {
  TCHAR szFile[MAX_PATH] = {0};

  OPENFILENAME ofn = {0};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = NULL;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = L"All Files\0*.*\0CSV Files\0*.csv\0";
  ofn.lpstrTitle = L"Select CSV file";
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if (GetOpenFileName(&ofn) == TRUE) {
    std::wstring ws(ofn.lpstrFile);
    std::string filePath(ws.begin(), ws.end());
    return filePath;
  }

  return "";
}

std::string SelectFolderAndSuggestFile(const std::string& suggested_fname) {
  HRESULT hr =
      CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
  std::string result;

  if (SUCCEEDED(hr)) {
    IFileSaveDialog* pFileSave;

    // Create the FileSaveDialog object.
    hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL,
                          IID_IFileSaveDialog,
                          reinterpret_cast<void**>(&pFileSave));

    if (SUCCEEDED(hr)) {
      // Set the suggested file name.
      std::wstring wsuggested_fname(suggested_fname.begin(),
                                    suggested_fname.end());
      pFileSave->SetFileName(wsuggested_fname.c_str());

      // Set the file type filters.
      COMDLG_FILTERSPEC rgSpec[] = {{L"Log Files", L"*.log"}};
      pFileSave->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);

      // Show the Save dialog box.
      hr = pFileSave->Show(NULL);

      // Get the file name from the dialog box.
      if (SUCCEEDED(hr)) {
        IShellItem* pItem;
        hr = pFileSave->GetResult(&pItem);
        if (SUCCEEDED(hr)) {
          PWSTR pszFilePath;
          hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

          // Convert the file path to std::string.
          if (SUCCEEDED(hr)) {
            std::wstring ws(pszFilePath);
            result = std::string(ws.begin(), ws.end());
            CoTaskMemFree(pszFilePath);
          }
          pItem->Release();
        }
      }
      pFileSave->Release();
    }
    CoUninitialize();
  }

  return result;
}

std::vector<ProtocolStep> readProtocolCSV(const std::string& filePath) {
  std::vector<ProtocolStep> protocolSteps;
  std::ifstream file(filePath);
  unsigned short i_step = 1; // Start with 1 for incremental ID starting with 1 (i.e. first step will be step 1 and not step 0)
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
    while (std::getline(ss, value, ',') && columnCount < 6) {
      try {
        int intValue = std::stoi(value);
        row.push_back(intValue);
      } catch (const std::invalid_argument& e) {
        std::cerr << "Invalid integer in CSV file: " << value << std::endl;
        continue;
      }
      columnCount++;
    }

    // Legacy mode: 5 columns. Then us mode is off (default).
    if (row.size() == 5) {
      ProtocolStep step(i_step, row[0], row[1], row[2], row[3], row[4], false);
      protocolSteps.push_back(step);
      i_step++;
    }
    else if (row.size() == 6) {  // New mode (since 2.1.0) with 6 columns, last column is us mode flag
      bool is_ns_mode = (row[5] != 0);
      ProtocolStep step(i_step, row[0], row[1], row[2], row[3], row[4],
                        is_ns_mode);
      protocolSteps.push_back(step);
      i_step++;
    } else {
      std::cerr << "Invalid number of columns in CSV file. Expected 5 or 6, got "
                << row.size() << std::endl;
      continue;
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

std::string getCurrentDateTime() {
  auto now = std::chrono::system_clock::now();
  std::time_t now_time = std::chrono::system_clock::to_time_t(now);

  std::tm now_tm;
  localtime_s(&now_tm, &now_time);

  std::ostringstream oss;
  oss << std::put_time(&now_tm, "%Y-%m-%d_%H-%M-%S");
  return oss.str();
}

std::string generateLogFileName(const std::string& prefix) {
  return prefix + getCurrentDateTime() + ".log";
}

void showOpenCSVInstructions() {
  std::cout << "Choose the CSV file with the protocol. Each row should have "
               "five entries:\n"
            << "1. LED index (0-5)\n"
            << "2. pulse duration (integer, ms)\n"
            << "3. time between pulses (integer, ms)\n"
            << "4. number of pulses (integer)\n"
            << "5. brightness (integer, 0 - 1000, 1000 = 100.0 %)\n"
            << "6. (optional) 1 if 'us mode' (pulse duration and time between pulses in us, multiples of 5 us), 0 if ms mode." << std::endl;
}

void intToCharArray(ViUInt16 number, char* charArray, size_t bufferSize) {
  // Convert integer to string
  std::string numberString = std::to_string(number);

  // Ensure the buffer is large enough
  if (numberString.length() < bufferSize) {
    // Manually copy each character
    for (size_t i = 0; i < numberString.length(); ++i) {
      charArray[i] = numberString[i];
    }
    // Add the null terminator
    charArray[numberString.length()] = '\0';
  } else {
    std::cerr << "Buffer size is too small to hold the number" << std::endl;
  }
}