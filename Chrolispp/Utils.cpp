#include <string>
#include <shlobj.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "ProtocolStep.hpp"
#include <windows.h>
#include <shobjidl.h>
#include <iomanip>
#include <chrono>
#include <ctime>



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
