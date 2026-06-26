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

  ## Tips for successful compilation
  * Make sure the vcxproj file (opened in a text editor like Notepad++) has all the include files in one (or more) `<ItemGroup>` entries, and that each file only appears once (with the correct relative path). The necessary h and hpp files should be in this repository's `include\external` folder.
  * Make sure under project properties (Right click on Chrolispp, NOT the solution) in the Solution Explorer window, C/C++ -> Additional Include Directories, and Linker -> Additional Library Directories point (absolute path) to this repository's `include\external` and `libs\external` folders.
  * Also make sure to add to PATH the `libs\external` folder (absolute path). Alternatively, the `C:\Program Files\IVI Foundation\VISA\Win64\Bin` folder (`Program Files (x86)` for 32-bit systems) should also work if Chrolis was installed with all the extra stuff (drivers).

## CMake
Currently, migration to CMake and cross-platform compatibility is under development. To build on Windows with CMake, do the following in the root repo folder:
1. `cmake -S . -B build -G "Visual Studio 17 2022" -A x64`
2. (optional) if VISA is installed in a non-default location:
  `cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  `-DCHROLISPP_VISA_BIN_DIR="C:/Program Files/IVI Foundation/VISA/Win64/Bin" `
  `-DCHROLISPP_VISA_LIB_DIR="C:/Program Files/IVI Foundation/VISA/Win64/Lib_x64/msc"`
3. Build with `cmake --build build --config Release`