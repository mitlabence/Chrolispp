// Chrolispp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

/***
  * Additional Include Paths:
  *   "C:\Program Files (x86)\IVI Foundation\VISA\WinNT\Include"    [32Bit]
  *   "C:\Program Files\IVI Foundation\VISA\Win64\Include"          [64Bit]
  *   "C:\Program Files\Thorlabs\upSERIES\Drivers\Instr\incl"       [No VXIPnP Folder]
  *
  * Additional Library Directories:
  *   "C:\Program Files (x86)\IVI Foundation\VISA\WinNT\lib\msc"    [32Bit]
  *   "C:\Program Files\IVI Foundation\VISA\Win64\Lib_x64\msc"      [64Bit]
  *   "C:\Program Files\Thorlabs\CHROLIS\Drivers\Instr\msvc"        [32Bit - No VXIPnP Folder]
  *   "C:\Program Files\Thorlabs\CHROLIS\Drivers\Instr\msvc64"      [64Bit - No VXIPnP Folder]
  *
  * Additional Dependencies:
  *   "TL6WL_32.lib"                                                [32Bit]
  *   "TL6WL_64.lib"                                                [64Bit]
  *
  * Library Locations:
  *   "C:\Program Files (x86)\IVI Foundation\VISA\WinNT\Bin"        [32Bit]
  *   "C:\Program Files\IVI Foundation\VISA\Win64\Bin"              [64Bit]
  *   "C:\Program Files\Thorlabs\CHROLIS\Drivers\Instr\bin"         [No VXIPnP Folder]
  *
  * Libraries:
  *   "TL6WL_32.dll"                                                [32Bit]
  *   "TL6WL_64.dll"                                                [64Bit]
 ***/

/*
* Given a csv file with the following columns in this order:
 * 1. LED index (0-5)
 * 2. Pulse length (ms)
 * 3. Time between pulses (ms)
 * 4. Number of pulses (integer)
 * 5. brightness (integer; 1000 is 100.0%, e.g. 555 is 55.5% power in Chrolis)
 * Each row with these columns corresponds to one step in the protocol.
 * These rows (steps) will be executed in order. The total time of one step is
 * n_pulses * (pulse_length + time_between_pulses) ms.
 * Breaks: If the pulse length is 0, it means a break of time_between_pulses ms. In this case, a single break
 * of length <time between pulses> is executed, disregarding all other values.
 */
#include "COMFunctions.hpp"
#include <Windows.h>
#include <stdio.h>
#include <string>
#include "TL6WL.h"
#include "LEDFunctions.hpp"
#include "Utils.hpp"
#include "ProtocolStep.hpp"
#include <iostream>
#include "Logger.hpp"
#define VERSION_STR "1.4.1"  // Version, change with each release!
#define LOGFNAME_PREFIX "stimlog_"  // beginning of log file name

const char dataLightOn[] =
    "6666";  // Arduino recognizes this and responds with "1"
const char dataOn[] = "1";
const char dataOff[] = "0";

int main(){
    bool arduinoFound = false;
    bool skipArduino = false;
    std::string comPort;
    HANDLE h_Serial = INVALID_HANDLE_VALUE;
    while (!arduinoFound &&
           !skipArduino) {  // break if arduino is found or user skips
      std::cout
          << "Enter Arduino COM port number (press enter to skip arduino):";
      std::cin >> comPort;
      comPort = "COM" + comPort;  // prepend COM to port number
      WCHAR* COM_PORT;
      COM_PORT = stringToWCHAR(comPort);
      h_Serial = createSerialHandle(COM_PORT);
      
      if (h_Serial == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening serial port" << std::endl;
        return -1;
      }
      try {
        configureSerialPort(h_Serial); 
        configureTimeoutSettings(h_Serial);
      } catch (const serial_port_config_error& e) {
        std::cerr << "Error configuring serial port: " << e.what() << std::endl;
        return -1;
      } catch (const timeout_setting_error& e) {
        std::cerr << "Error configuring timeout settings: " << e.what()
                  << std::endl;
        return -1;
      } catch (const com_io_error& e) {
        std::cerr << "Error writing to/reading from serial port: " << e.what()
                  << std::endl;
        return -1;
      } 
      catch (const std::exception& e) {
        std::cerr << "Error configuring serial port: " << e.what() << std::endl;
        return -1;
      }
      // Write message
      try {
        writeMessage(h_Serial, dataLightOn, sizeof(dataLightOn)); 
      } catch (const com_io_error& e) {
        std::cerr << "Error writing to serial port: " << e.what() << std::endl;
        return -1;
      }
      // read message
      try {
        char* arduinoResponse = readMessage(h_Serial, 1);
        if (arduinoResponse[0] == 1) {
          arduinoFound = true;
        }
      } catch (const com_io_error& e) {
        std::cerr << "Error reading from serial port: " << e.what()
                  << std::endl;
        return -1;
      }
    }
    if (arduinoFound) {
      std::cout << "Arduino detected." << std::endl;   
    } else {
      std::cout << "Skipping Arduino connection." << std::endl;
    }
    showOpenCSVInstructions();
  std::string suggested_log_fname = generateLogFileName(LOGFNAME_PREFIX);

  std::string fpath = BrowseCSV();
  
  if (fpath.empty()) {
    std::cerr << "No file selected." << std::endl;
    return -1;
  }
  if (!isCSVFile(fpath)) {
    std::cerr << "The selected file is not a CSV file." << std::endl;
    return -1;
  }
  std::vector<ProtocolStep> protocolSteps = readProtocolCSV(fpath);
  if (protocolSteps.size() == 0) {
    std::cerr << "No protocol steps found in the CSV file." << std::endl;
    return -1;
  }
  // Select output log file path and name
  std::string fpath_log 
= SelectFolderAndSuggestFile(suggested_log_fname);
  if (fpath_log.empty()) {
    std::cerr << "No log file selected." << std::endl;
    return -1;
  }
  // Set up logging
  std::unique_ptr<Logger> logger; // for accessing the logger outside try
  try {
    logger = std::make_unique<Logger>(fpath_log);
    // Print log file path
    std::cout << "Starting log file " << fpath_log << std::endl;
    // TODO: add logging calls, check if info and error can be differentiated!
    logger->info("Logging started. Code version: " + std::string(VERSION_STR));
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  logger->info("Protocol file: " + fpath);
  logger->info("Arduino used: " + arduinoFound ? "true" : "false");
  // Sanity checking the protocol steps
  char err_buffer[40];
  for (int i_step = 0; i_step < protocolSteps.size(); i_step++) {
    ProtocolStep& step = protocolSteps[i_step];
    // Check if the LED index is in the correct range
    if (step.led_index < 0 || step.led_index > 5) {
      sprintf_s(err_buffer, "Invalid LED index in step %d", i_step + 1);
      logger->error(err_buffer);
      std::cerr<< err_buffer <<std::endl;
      return -1;
    }
    // Check if the pulse length is > 0 (0 means break) and not too large (< 6 h =
    // 21600000 ms)
    if (step.pulse_width_ms < 0 || step.pulse_width_ms > 21600000) {
      sprintf_s(err_buffer, "Invalid pulse length in step %d", i_step + 1);
      logger->error(err_buffer);
      std::cerr << err_buffer <<std::endl;
      return -1;
    }
    // Check if the time between pulses is not negative and not too large (< 6 h =
    // 21600000 ms)
    if (step.time_between_pulses_ms < 0 ||
        step.time_between_pulses_ms > 21600000) {
      sprintf_s(err_buffer, "Invalid time between pulses in step %d",
                   i_step + 1);
      logger->error(err_buffer);
      std::cerr << err_buffer << std::endl;
      return -1;
    }
    // Check corner case: break of length 0 (both pulse length and time between
    // pulses are set to 0
    if (step.pulse_width_ms == 0 && step.time_between_pulses_ms == 0) {
      sprintf_s(err_buffer, "Invalid step %d: No action.",
                   i_step + 1);
      logger->error(err_buffer);
      std::cerr << err_buffer << std::endl;
      return -1;
    }
    // Check if the number of pulses is positive and not too large (< 10000)
    if (step.n_pulses <= 0 || step.n_pulses > 10000) {
      sprintf_s(err_buffer, "Invalid number of pulses in step %d", i_step + 1);
      logger->error(err_buffer);
      std::cerr << err_buffer << std::endl;
      return -1;
    }
    // Print step for user to review
    std::cout << "Step " << i_step + 1 << ":\n";
    step.printStep();
  }
  // Wait for user to press space to start the protocol, or q to quit.
  std::cout << "\nPress y + enter to start the protocol, or q + enter to quit." << std::endl;
  char c = ' ';
  while (c != 'y' && c != 'q') {
    c = getchar();
  }
  if (c == 'q') {
    logger->info("User entered q, closing application.");
    return 0;
  }
  if (c == 'y') {
    logger->info("User entered y, preparing to start protocol...");
    std::cout << "Starting protocol." << std::endl;
  }

    ViStatus err;
#ifdef WIN32
    ViChar bitness[TL6WL_LONG_STRING_SIZE] = "x86";
#else
    ViChar bitness[TL6WL_LONG_STRING_SIZE] = "x64";
#endif
    logger->info("Bitness is " + std::string(bitness));
    printf("This is start of Thorlabs CHROLIS TL6WL logging output [%s]!\n\n",
           bitness);

    printf("START : Chrolis++\n");

    printf("Search for connected CHROLIS Devices\n");
    logger->info("Search for connected CHROLIS Devices.");
    ViSession instr = 0;
    ViUInt32 rsrcCnt = 0;
    err = TL6WL_findRsrc(instr, &rsrcCnt);
    if (rsrcCnt == 0) {
      logger->error("No devices found. Protocol aborted.");
      printf("No devices found. Protocol aborted.\n");
      return -1;
    }
    printf("Found '%lu' Devices\n" , rsrcCnt);
    if (VI_SUCCESS != err)
    {
        sprintf_s(err_buffer, "TL6WL_findRsrc() : Error Code = %#.8lX",
                       err);
        logger->error(err_buffer);
        printf("  TL6WL_findRsrc() :\n    Error Code = %#.8lX\n" , err);
        printf("\nProtocol Terminated\n");
        return -1;
    }
    

    printf("\nGet Information of found Devices\n");
    ViChar resourceName[512] = "n/a";
    ViChar modelName[TL6WL_LONG_STRING_SIZE] = "n/a";
    ViChar serialNumber[TL6WL_LONG_STRING_SIZE] = "n/a";
    ViChar manufacturer[TL6WL_LONG_STRING_SIZE] = "n/a";
    ViBoolean resourceAvailable = VI_FALSE;
    ViBoolean IDQuery = VI_FALSE;
    ViBoolean resetDevice = VI_FALSE;
    for (ViInt32 i = 0; rsrcCnt > i; ++i)
    {
        err = TL6WL_getRsrcName(instr , i , resourceName);
        if (VI_SUCCESS == err)
        {
            printf("    Resource Name DeviceID[%lu] = <%s>\n" , 1 + i, resourceName);

            err = TL6WL_getRsrcInfo(instr , i , modelName , serialNumber , manufacturer , &resourceAvailable);
            if (VI_SUCCESS == err)
            {
                printf("      Model Name = <%s>\n      SerNo = <%s>\n      Manufacturer = <%s>\n      Available = <%s>\n" ,
                       modelName , serialNumber , manufacturer , resourceAvailable == VI_TRUE ? "Yes" : "No");
            }
            else
            {
                printf("  TL6WL_getRsrcInfo() :    Error Code = %#.8lX\n" , err);
            }
        }
        else
        {
          sprintf_s(err_buffer, "TL6WL_getRsrcName() : Error Code = %#.8lX",
                       err);
            printf("  TL6WL_getRsrcName() :\n    Error Code = %#.8lX\n" , err);
        }
    }

    printf("\nOpen first available Device found\n");
    logger->info("Opening device: " + std::string(resourceName));
    err = TL6WL_getRsrcName(instr , 0 , resourceName);
    if (VI_SUCCESS != err)
    {
      sprintf_s(err_buffer, "TL6WL_getRsrcName() : Error Code = %#.8lX",
                   err);
        printf("  TL6WL_getRsrcName() :\n    Error Code = %#.8lX\n" , err);
        printf("\nProtocol Terminated\n");
        return -2;
    }
    logger->info("Initializing device: " + std::string(resourceName));
    err = TL6WL_init(resourceName , IDQuery , resetDevice , &instr);
    if (VI_SUCCESS != err)
    {
      sprintf_s(err_buffer, "TL6WL_init() : Error Code = %#.8lX", err);
        printf("  TL6WL_init() :\n    Error Code = %#.8lX\n" , err);
        printf("\nProtocol Terminated\n");
        return -3;
    }

    logger->info("Reading Box Status Register.");
    printf("\nRead Box Status Register\n");

    ViUInt32 boxStatus;
    err = TL6WL_getBoxStatus(instr , &boxStatus);
    try {
      std::string boxWarning = readBoxStatusWarnings(boxStatus); 
      // if length of string > 0, there was a warning
      if (!boxWarning.empty()) {
        logger->warning(boxWarning);
        std::cerr << boxWarning << std::endl;
      } else {
        logger->info("Box status OK.");
      }
    } catch (const std::exception& e) {
      logger->error(e.what());
      std::cerr << e.what() << std::endl;
      return -1;
    }
    
    std::cout << "\n" << std::endl;
    int i_step = 0;
    size_t n_steps = protocolSteps.size();
    logger->info("Starting protocol with " + std::to_string(n_steps) +
                " steps.");
    for (ProtocolStep& step : protocolSteps) {
       std::cout << "step " << i_step+1 << "/" << n_steps << std::endl;
      char* stepChars = step.stepToChars();
       logger->info("Step " + std::to_string(i_step) + ": " + stepChars);
      std::cout << stepChars << std::endl;
       LED_PulseNTimesWithArduino(instr, step.led_index, step.pulse_width_ms,
                      step.time_between_pulses_ms, step.n_pulses,
                      step.brightness, h_Serial);
      logger->info("Step " + std::to_string(i_step) + " done.");
      i_step++;
    }
    
    logger->info("Closing device.");
    printf("\nClose Device\n");
    err = TL6WL_close(instr);
    if (VI_SUCCESS != err)
    {
      sprintf_s(err_buffer, "TL6WL_close() : Error Code = %#.8lX", err);
      logger->error(err_buffer);
        printf("  TLUP_close() :\n    Error Code = %#.8lX\n" , err);
    }
    logger->info("Device closed.");
    printf("\nProtocol Ended.\n");
    while (getchar() != 'q') {
      std::cout << '\n' << "Press q then enter to quit...";
    }
    logger->info("Application closed.");
    return 0;
}





// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

