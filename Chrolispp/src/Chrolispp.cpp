// Chrolispp.cpp : This file contains the 'main' function. Program execution
// begins and ends there.
//

/***
 * Additional Include Paths:
 *   "C:\Program Files (x86)\IVI Foundation\VISA\WinNT\Include"    [32Bit]
 *   "C:\Program Files\IVI Foundation\VISA\Win64\Include"          [64Bit]
 *   "C:\Program Files\Thorlabs\upSERIES\Drivers\Instr\incl"       [No VXIPnP
 *Folder]
 *
 * Additional Library Directories:
 *   "C:\Program Files (x86)\IVI Foundation\VISA\WinNT\lib\msc"    [32Bit]
 *   "C:\Program Files\IVI Foundation\VISA\Win64\Lib_x64\msc"      [64Bit]
 *   "C:\Program Files\Thorlabs\CHROLIS\Drivers\Instr\msvc"        [32Bit - No
 *VXIPnP Folder] "C:\Program Files\Thorlabs\CHROLIS\Drivers\Instr\msvc64" [64Bit
 *- No VXIPnP Folder]
 *
 * Additional Dependencies:
 *   "TL6WL_32.lib"                                                [32Bit]
 *   "TL6WL_64.lib"                                                [64Bit]
 *
 * Library Locations:
 *   "C:\Program Files (x86)\IVI Foundation\VISA\WinNT\Bin"        [32Bit]
 *   "C:\Program Files\IVI Foundation\VISA\Win64\Bin"              [64Bit]
 *   "C:\Program Files\Thorlabs\CHROLIS\Drivers\Instr\bin"         [No VXIPnP
 *Folder]
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
 * Breaks: If the pulse length is 0, it means a break of time_between_pulses ms.
 * In this case, a single break of length <time between pulses> is executed,
 * disregarding all other values.
 */
// TODO: show error message if wrong arduino COM Port specified
#include <Windows.h>
#include <conio.h>  // for key-press mode
#include <stdio.h>

#include <csignal>
#include <iostream>
#include <string>
#include <thread>

#include "COMFunctions.hpp"
#include "LEDFunctions.hpp"
#include "Logger.hpp"
#include "ProtocolStep.hpp"
#include "TL6WL.h"
#include "Utils.hpp"

constexpr auto VERSION_STR = "1.4.4";  // Version, change with each release!;
constexpr auto LOGFNAME_PREFIX = "stimlog_";  // beginning of log file name;

constexpr auto CMD_COM_CHECK =
    6666;  // Command word: Arduino recognizes this and responds with "1";
constexpr auto CMD_LIGHT_OFF = 1;  // Command word: set Arduino output to 0

struct CleanupContext {
  ViSession instr;
  HANDLE h_Serial;
  std::unique_ptr<Logger>* logger;
};

CleanupContext cleanupContext;

static void cleanup(ViSession instr, HANDLE h_Serial,
             std::unique_ptr<Logger>* logger) {
  std::cout << "Cancelling...";
  Logger* logger_ptr = logger->get();
  logger_ptr->info("Interrupted...");
  // set all LEDs to 0, close LED connection and logger.
  std::cout << "Turning off LEDs and closing connection." << std::endl;
  TL6WL_setLED_HeadPowerStates(instr, VI_FALSE, VI_FALSE, VI_FALSE, VI_FALSE,
                               VI_FALSE, VI_FALSE);
  TL6WL_close(instr);
  // Send 1 to Arduino to turn off pulse
  if (h_Serial != INVALID_HANDLE_VALUE) {
    char message[5] = {0}; // initialize to 0s
    intToCharArray(CMD_LIGHT_OFF, message, sizeof(message));
    writeMessage(h_Serial, message, sizeof(message));
    CloseHandle(h_Serial);
  }
  // TODO: add pointer check (if (ptr && ptr->get())

  logger_ptr->info("LEDs turned off, connection closed.");
}

static void signalHandler(int signal) {
  if (signal == SIGINT) {
    cleanup(cleanupContext.instr, cleanupContext.h_Serial,
            cleanupContext.logger);
    std::cout << "Interrupted." << std::endl;
    std::exit(0);
  }
}

int main() {
  std::cout << "Chrolis++ version " << VERSION_STR << std::endl;
  bool arduinoFound = false;
  char* firmwareVersion = new char[1];  // 1 byte, can hold -127 to 127?
  firmwareVersion[0] = static_cast<char>(-1);
  bool skipArduino = false;
  int dac_resolution_bits =
      0;  // if stays 0, no communication with Arduino will happen. If 1,
          // digital output, if >1, DAC with specified resolution.
  std::string comPort;
  HANDLE h_Serial = INVALID_HANDLE_VALUE;
  // whether the user wants to do key-press mode: pressing a specific key
  // starts an LED pattern. If false and program is not interrupted,
  // the program will read the csv file and play the steps, i.e. run in protocol
  // mode.
  bool keyPressMode = false;
  while (!arduinoFound &&
         !skipArduino) {  // break if arduino is found or user skips
    std::cout << "Enter Arduino COM port number (enter n to skip arduino):";
    std::cin >> comPort;
    if (comPort == "n") {
      skipArduino = true;
      break;
    }
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
    } catch (const std::exception& e) {
      std::cerr << "Error configuring serial port: " << e.what() << std::endl;
      return -1;
    }
    // Write message
    try {
      char message[5];
      intToCharArray(CMD_COM_CHECK, message, sizeof(message));
      writeMessage(h_Serial, message, sizeof(message));
    } catch (const com_io_error& e) {
      std::cerr << "Error writing to serial port: " << e.what() << std::endl;
      return -1;
    }
    // read message, set up firmware-specific behavior
    try {
      firmwareVersion = readMessage(h_Serial, 1);
      if (firmwareVersion[0] == 1) {  // only digital signals
        arduinoFound = true;
        dac_resolution_bits = 1;
      } else if (firmwareVersion[0] ==
                 2) {  // Arduino Uno built-in PWM, 8-bit resolution
        arduinoFound = true;
        dac_resolution_bits = 8;
      } else if (firmwareVersion[0] == 3) {  // MCP4725 DAC, 12-bit resolution
        arduinoFound = true;
        dac_resolution_bits = 12;
      } else {
        std::cerr << "Invalid firmware version from Arduino: "
                  << std::to_string(firmwareVersion[0]) << std::endl;
        return -1;
      }
    } catch (const com_io_error& e) {
      std::cerr << "Error reading from serial port: " << e.what() << std::endl;
      return -1;
    }
  }
  if (arduinoFound) {
    std::cout << "Arduino detected with firmware ID: "
              << std::to_string(firmwareVersion[0]) << std::endl;
  } else {
    std::cout << "Skipping Arduino connection." << std::endl;
  }
  showOpenCSVInstructions();
  std::string suggested_log_fname = generateLogFileName(LOGFNAME_PREFIX);

  std::string fpath = BrowseCSV();

  if (fpath.empty()) {
    std::cerr << "No file selected." << std::endl;
    // Ask the user if they want to continue in key-press mode
    std::cout << "Do you want to continue in key-press mode? (y/n): ";
    char response;
    std::cin >> response;

    if (response == 'y' || response == 'Y') {
      keyPressMode = true;
      std::cout << "Key-press mode enabled." << std::endl;
    } else {
      return -1;  // Quit if the user does not want to continue
    }
  }
  std::string modeString = keyPressMode ? "key-press" : "protocol";
  std::vector<ProtocolStep> protocolSteps;
  if (!keyPressMode) {
    if (!isCSVFile(fpath)) {
      std::cerr << "The selected file is not a CSV file." << std::endl;
      return -1;
    }
    protocolSteps = readProtocolCSV(fpath);
    if (protocolSteps.size() == 0) {
      std::cerr << "No protocol steps found in the CSV file." << std::endl;
      return -1;
    }
  }

  // Select output log file path and name
  std::string fpath_log = SelectFolderAndSuggestFile(suggested_log_fname);
  if (fpath_log.empty()) {
    std::cerr << "No log file selected." << std::endl;
    return -1;
  }
  // Set up logging
  std::unique_ptr<Logger> logger;  // for accessing the logger outside try
  try {
    logger = std::make_unique<Logger>(fpath_log);
    // Print log file path
    std::cout << "Starting log file " << fpath_log << std::endl;
    // TODO: add logging calls, check if info and error can be differentiated!
    logger->info("Logging started. Code version: " + std::string(VERSION_STR));
    // write mode (key-press or protocol mode) into log file
    logger->info("Mode: " + modeString);
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  std::string arduino_found_string = arduinoFound ? "true" : "false";
  logger->info("Arduino used: " + arduino_found_string);
  if (arduinoFound) {
    logger->info("Arduino firmware version: " +
                 std::to_string(firmwareVersion[0]));
  }
  char err_buffer[40];  // buffer for storing error messages that will be
                        // printed to logging file
  if (!keyPressMode) {
    logger->info("Protocol file: " + fpath);
    // Sanity checking the protocol steps
    if (protocolSteps.empty()) {
      logger->error(
          "Protocol steps are empty. Might be due to bad initialization of the "
          "protocolSteps variable.");
    }
    for (int i_step = 0; i_step < protocolSteps.size(); i_step++) {
      ProtocolStep& step = protocolSteps[i_step];
      // Check if the LED index is in the correct range
      if (step.led_index < 0 || step.led_index > 5) {
        sprintf_s(err_buffer, "Invalid LED index in step %d", i_step + 1);
        logger->error(err_buffer);
        std::cerr << err_buffer << std::endl;
        return -1;
      }
      // Check if the pulse length is > 0 (0 means break) and not too large (< 6
      // h = 21600000 ms)
      if (step.pulse_width_ms < 0 || step.pulse_width_ms > 21600000) {
        sprintf_s(err_buffer, "Invalid pulse length in step %d", i_step + 1);
        logger->error(err_buffer);
        std::cerr << err_buffer << std::endl;
        return -1;
      }
      // Check if the time between pulses is not negative and not too large (< 6
      // h = 21600000 ms)
      if (step.time_between_pulses_ms < 0 ||
          step.time_between_pulses_ms > 21600000) {
        sprintf_s(err_buffer, "Invalid time between pulses in step %d",
                  i_step + 1);
        logger->error(err_buffer);
        std::cerr << err_buffer << std::endl;
        return -1;
      }
      // Check corner case: break of length 0 (both pulse length and time
      // between pulses are set to 0
      if (step.pulse_width_ms == 0 && step.time_between_pulses_ms == 0) {
        sprintf_s(err_buffer, "Invalid step %d: No action.", i_step + 1);
        logger->error(err_buffer);
        std::cerr << err_buffer << std::endl;
        return -1;
      }
      // Check if the number of pulses is positive and not too large (< 10000)
      if (step.n_pulses <= 0 || step.n_pulses > 10000) {
        sprintf_s(err_buffer, "Invalid number of pulses in step %d",
                  i_step + 1);
        logger->error(err_buffer);
        std::cerr << err_buffer << std::endl;
        return -1;
      }
      // Print step for user to review
      std::cout << "Step " << i_step + 1 << ":\n";
      step.printStep();
    }
  }
  // Wait for user to press space to start the protocol or key-press mode, or q
  // to quit.
  std::cout << "\nPress y + enter to start" << modeString
            << " mode, or q then enter to quit." << std::endl;
  char c = ' ';
  while (c != 'y' && c != 'q') {
    c = getchar();
  }
  if (c == 'q') {
    logger->info("User entered q, closing application.");
    return 0;
  }
  if (c == 'y') {
    logger->info("User entered y, preparing to start...");
    std::cout << "Starting " << modeString << " mode." << std::endl;
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
  printf("Found '%lu' Devices\n", rsrcCnt);
  if (VI_SUCCESS != err) {
    sprintf_s(err_buffer, "TL6WL_findRsrc() : Error Code = %#.8lX", err);
    logger->error(err_buffer);
    printf("  TL6WL_findRsrc() :\n    Error Code = %#.8lX\n", err);
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
  for (ViInt32 i = 0; rsrcCnt > i; ++i) {
    err = TL6WL_getRsrcName(instr, i, resourceName);
    if (VI_SUCCESS == err) {
      printf("    Resource Name DeviceID[%lu] = <%s>\n", 1 + i, resourceName);

      err = TL6WL_getRsrcInfo(instr, i, modelName, serialNumber, manufacturer,
                              &resourceAvailable);
      if (VI_SUCCESS == err) {
        printf(
            "      Model Name = <%s>\n      SerNo = <%s>\n      Manufacturer = "
            "<%s>\n      Available = <%s>\n",
            modelName, serialNumber, manufacturer,
            resourceAvailable == VI_TRUE ? "Yes" : "No");
      } else {
        printf("  TL6WL_getRsrcInfo() :    Error Code = %#.8lX\n", err);
      }
    } else {
      sprintf_s(err_buffer, "TL6WL_getRsrcName() : Error Code = %#.8lX", err);
      printf("  TL6WL_getRsrcName() :\n    Error Code = %#.8lX\n", err);
    }
  }

  printf("\nOpen first available Device found\n");
  logger->info("Opening device: " + std::string(resourceName));
  err = TL6WL_getRsrcName(instr, 0, resourceName);
  if (VI_SUCCESS != err) {
    sprintf_s(err_buffer, "TL6WL_getRsrcName() : Error Code = %#.8lX", err);
    printf("  TL6WL_getRsrcName() :\n    Error Code = %#.8lX\n", err);
    printf("\nProtocol Terminated\n");
    return -2;
  }
  logger->info("Initializing device: " + std::string(resourceName));
  err = TL6WL_init(resourceName, IDQuery, resetDevice, &instr);
  if (VI_SUCCESS != err) {
    sprintf_s(err_buffer, "TL6WL_init() : Error Code = %#.8lX", err);
    printf("  TL6WL_init() :\n    Error Code = %#.8lX\n", err);
    printf("\nProtocol Terminated\n");
    return -3;
  }

  logger->info("Reading Box Status Register.");
  printf("\nRead Box Status Register\n");

  ViUInt32 boxStatus;
  err = TL6WL_getBoxStatus(instr, &boxStatus);
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
  // Start thread to listen for escape key
  cleanupContext.h_Serial = h_Serial;
  cleanupContext.instr = instr;
  cleanupContext.logger = &logger;

  // Update the assignment to match the new type
  std::signal(SIGINT, signalHandler);
  if (keyPressMode) {
    std::cout << "Press Ctrl+C to quit the key-press mode." << std::endl;
    // TODO: add listener to one button, say, S, to stim for 4 s with LED 0.
    //      If it works, add control as well.
    // TODO: pressing S multiple times creates a queue of multiple stims! Might
    // need to avoid this somehow (accidental pressing twice).
    //  Quit after one stim?
    while (true) {
      if (_kbhit()) {        // Check if a key was pressed
        char ch = _getch();  // Read the key (without Enter)
        if (ch == 'S' || ch == 's') {
          std::cout << "LED 0: Stimulating for 4 seconds." << std::endl;
          logger->info("LED 0: Stimulating for 4 seconds.");
          LED_PulseNTimesWithArduino(instr, 0, 4000, 0, 1, 100, h_Serial,
                                     dac_resolution_bits);
          logger->info("LED 0: Done.");
        } else if (ch == 'Q' || ch == 'q') {
          // TODO: test this quit method. If works, then sigint can be limited
          // to protocol mode? Also, if this works, then q + enter in other
          // cases can be replaced.
          break;  // Exit loop
        }
      }
    }
  } else {  // Run protocol steps
    std::cout << "Press Ctrl+C to cancel the protocol." << std::endl;
    int i_step = 0;
    size_t n_steps = protocolSteps.size();
    logger->info("Starting protocol with " + std::to_string(n_steps) +
                 " steps.");
    for (ProtocolStep& step : protocolSteps) {
      std::cout << "step " << i_step + 1 << "/" << n_steps << std::endl;
      char* stepChars = step.stepToChars();
      logger->info("Step " + std::to_string(i_step) + ": " + stepChars);
      std::cout << stepChars << std::endl;
      LED_PulseNTimesWithArduino(instr, step.led_index, step.pulse_width_ms,
                                 step.time_between_pulses_ms, step.n_pulses,
                                 step.brightness, h_Serial,
                                 dac_resolution_bits);
      logger->info("Step " + std::to_string(i_step) + " done.");
      i_step++;
    }
  }

  logger->info("Closing device.");
  printf("\nClose Device\n");
  err = TL6WL_close(instr);
  if (VI_SUCCESS != err) {
    sprintf_s(err_buffer, "TL6WL_close() : Error Code = %#.8lX", err);
    logger->error(err_buffer);
    printf("  TLUP_close() :\n    Error Code = %#.8lX\n", err);
  }
  logger->info("Device closed.");
  printf("\nProtocol Ended.\n");
  while (getchar() != 'q') {
    std::cout << '\n' << "Press q then enter to quit...";
  }
  logger->info("Application closed.");
  delete[] firmwareVersion;
  return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started:
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add
//   Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project
//   and select the .sln file
