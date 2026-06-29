# Chrolis++
Custom CSV-based protocol for the ThorLabs Chrolis 6-wavelength LED source. [Chrolispp-planner](https://github.com/mitlabence/chrolispp-planner) is a related tool aimed at simplifying the assembly of illumination protocols.
## CSV format
The CSV file should have 5 columns. Each row defines a sequence of light pulses of the same LED, and these sequences are executed one after the other. The rows in order:
* The LED index (0-5)
* Duration of each individual light pulse (integer, ms; stored as (signed) long)
* Time between pulses (integer, ms, (signed) long)
* Number of pulses (integer, (signed) long)
* Brightness (integer, 0-1000, where 1000 is 100.0%; e.g. 123 is 12.3%, same control as in the Chrolis application)
* (Optional since 2.1.0) 1 if "us mode" (duration of light pulses and time between pulses should be interpreted as us, not ms; then values should be multiples of 5), 0 if "ms mode" (duration and time between pulses to be interpreted as ms).

# Prerequisites
1. * Visual Studio build tools: either install Microsoft Visual Studio, check Desktop Development with C++, and make sure MSVC v143 - 2022 C++ x64/x86 build tools as well as Windows 11 SDK are included. If only using VS to compile, C++ CMake Tools should also be included.
  * Alternatively, get Build Tools for Visual Studio (without the IDE), and select the same components as above.
2. If planning to work from the command line, CMake might be necessary as well (although it can be included in both VS downloads).

# Compilation
## VS
1. Open the folder (it is not a solution, as there is no `.sln` file).
2. If not run automatically, choose `Project > Configure Chrolispp`
3. Once generation is complete, building `Chrolispp.exe` should be possible either as `Build > Build Chrolispp.exe`, or the same command seen as a green arrow (or `Ctrl + B`).

## CLI (CMake)
The project was migrated to build with CMake, but still relying on VS as the generator (`-G` flag). In the root directory:
1. `cmake -S . -B build -G "Visual Studio 17 2022" -A x64`
2. (optional, should not be necessary) if VISA is installed in a non-default location:
  `cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  `-DCHROLISPP_VISA_BIN_DIR="C:/Program Files/IVI Foundation/VISA/Win64/Bin" `
  `-DCHROLISPP_VISA_LIB_DIR="C:/Program Files/IVI Foundation/VISA/Win64/Lib_x64/msc"`
3. Build with `cmake --build build --config Release` (or with `--config Debug`)