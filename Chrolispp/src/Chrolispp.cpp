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
// FIXME: for single_pulses_quick_2color.csv, Error determining end of batch:
// batch_end <= step_cursor. Batch_id: 2, batch_end: -1, step_cursor: 2
/*
single_pulses_quick.csv:
2025-10-17 17:06:27.774	[ERROR]	Error determining end of batch: batch_end <=
step_cursor. Batch_id: 1, batch_end: 0, step_cursor: 0
2025-10-17 17:06:27.774 [ERROR]	Error determining end of batch: batch_end <=
step_cursor. Batch_id: 2, batch_end: 1, step_cursor: 1 2025-10-17 17:06:27.774
[ERROR]	Error determining end of batch: batch_end <= step_cursor. Batch_id: 3,
batch_end: 2, step_cursor: 2 2025-10-17 17:06:27.776	[ERROR]	Error
determining end of batch: batch_end <= step_cursor. Batch_id: 4, batch_end: -1,
step_cursor: 3

*/
// TODO: update keypress mode!
//  TODO: show error message if wrong arduino COM Port specified
//  FIXME: waiting too long to open a file might crash the software.
//  FIXME: "Press y + enter to startprotocol  mode, or q then enter to quit."
//  TODO: move logging into async (start new thread) to readuce latency?
//  TODO: add Arduino as a communications queue (each queue element should be
//  executed immediately... so in best case, queue will be always directly
//  emptied).
// TODO: unify variable naming convention (either camelCase or snake_case)
// TODO: Ctrl+C does not stop arduino! Need to press reset on it...

#include <Windows.h>
#include <conio.h>  // for key-press mode
#include <stdio.h>

#include <csignal>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "ArduinoCommands.hpp"
#include "COMFunctions.hpp"
#include "LEDFunctions.hpp"
#include "Logger.hpp"
#include "ProtocolPlanner.hpp"
#include "ProtocolStep.hpp"
#include "TL6WL.h"
#include "Timing.hpp"
#include "Utils.hpp"

constexpr auto VERSION_STR = "2.0.0";  // Version, change with each release!;
constexpr auto LOGFNAME_PREFIX = "stimlog_";  // beginning of log file name;
constexpr bool USE_BOB =
    true;  // whether to use the breakout board for timing signals

struct CleanupContext {
  ViSession instr;
  HANDLE h_Serial;
  std::unique_ptr<Logger>* logger;
};

CleanupContext cleanupContext;

static ViStatus cleanup(ViSession instr, HANDLE h_Serial,
                        std::unique_ptr<Logger>* logger) {
  ViStatus err;
  Logger* logger_ptr = logger->get();
  logger_ptr->trace("cleanup()");
  std::cout << "Cleaning up...";
  // set all LEDs to 0, close LED connection and logger.
  std::cout << "Turning off LEDs and closing connection." << std::endl;
  TL6WL_TU_StartStopGeneratorOutput_TU(
      instr, VI_FALSE);  // Stop the internal timer in case it is running
  TL6WL_setLED_HeadPowerStates(instr, VI_FALSE, VI_FALSE, VI_FALSE, VI_FALSE,
                               VI_FALSE, VI_FALSE);
  err = TL6WL_close(instr);
  // Send 1 to Arduino to turn off pulse
  if (h_Serial != INVALID_HANDLE_VALUE) {
    sendCommandToArduino(h_Serial, RESET);
    CloseHandle(h_Serial);
  }
  // TODO: add pointer check (if (ptr && ptr->get())
  logger_ptr->info("LEDs turned off, connection closed.");
  logger_ptr->flush();
  return err;
}

static void signalHandler(int signal) {
  if (signal == SIGINT) {
    cleanup(cleanupContext.instr, cleanupContext.h_Serial,
            cleanupContext.logger);
    std::cout << "Interrupted." << std::endl;
    std::exit(0);
  }
}
// TODO: add more errors if arduino com port is incorrect (no arduino on that
// port)
// TODO: try to solve Arduino detection issue (wrong firmware detected):
// https://copilot.microsoft.com/shares/cgkxsbhBYHzPpfGi2PJpM
int main() {
  std::cout << "Chrolis++ version " << VERSION_STR << std::endl;
  ViStatus err;
  char err_buffer[40];  // buffer for storing error messages that will be
                        // printed to logging file
#ifdef WIN32
  ViChar bitness[TL6WL_LONG_STRING_SIZE] = "x86";
#else
  ViChar bitness[TL6WL_LONG_STRING_SIZE] = "x64";
#endif
  // printf("Search for connected CHROLIS Devices\n");
  // logger->info("Search for connected CHROLIS Devices.");
  ViSession instr = 0;
  ViUInt32 rsrcCnt = 0;
  //std::wstring COM_PORT;
  WCHAR* COM_PORT;
  bool arduinoFound = false;
  uint8_t firmwareVersion = 0;
  bool skipArduino = false;
  int dac_resolution_bits =
      0;  // if stays 0, no communication with Arduino will happen. If 1,
          // digital output, if >1, DAC with specified resolution.
  std::string comPort;
  HANDLE h_Serial = INVALID_HANDLE_VALUE;

  // whether the user wants to do key-press mode: pressing a specific key
  // starts an LED pattern. If false and program is not interrupted,
  // the program will read the csv file and play the steps, i.e. run in
  // protocol mode.
  bool keyPressMode = false;
  bool skipChrolis = false;
  bool isDebug = false;  // When no Chrolis device is available but want to
                         // debug ProtocolPlanner
  if (isDebug) {
    std::cout << "Debug mode detected. Ignoring Chrolis and Arduino steps..."
              << std::endl;
  } else {
    if (!skipChrolis) {
      err = TL6WL_findRsrc(instr, &rsrcCnt);

      if (rsrcCnt == 0) {
        // logger->error("No devices found. Protocol aborted.");
        printf("No Chrolis LED device found.\n");
        return -1;
      }
      printf("Found '%lu' Devices\n", rsrcCnt);
      if (VI_SUCCESS != err) {
        sprintf_s(err_buffer, "TL6WL_findRsrc() : Error Code = %#.8lX", err);
        // logger->error(err_buffer); // At this point, no log file has been
        // selected yet
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
          printf("    Resource Name DeviceID[%lu] = <%s>\n", 1 + i,
                 resourceName);

          err = TL6WL_getRsrcInfo(instr, i, modelName, serialNumber,
                                  manufacturer, &resourceAvailable);
          if (VI_SUCCESS == err) {
            printf(
                "      Model Name = <%s>\n      SerNo = <%s>\n      "
                "Manufacturer "
                "= "
                "<%s>\n      Available = <%s>\n",
                modelName, serialNumber, manufacturer,
                resourceAvailable == VI_TRUE ? "Yes" : "No");
          } else {
            printf("  TL6WL_getRsrcInfo() :    Error Code = %#.8lX\n", err);
          }
        } else {
          sprintf_s(err_buffer, "TL6WL_getRsrcName() : Error Code = %#.8lX",
                    err);
          printf("  TL6WL_getRsrcName() :\n    Error Code = %#.8lX\n", err);
        }
      }

      printf("\nOpen first available Device found\n");
      // logger->info("Opening device: " + std::string(resourceName));
      err = TL6WL_getRsrcName(instr, 0, resourceName);
      if (VI_SUCCESS != err) {
        sprintf_s(err_buffer, "TL6WL_getRsrcName() : Error Code = %#.8lX", err);
        printf("  TL6WL_getRsrcName() :\n    Error Code = %#.8lX\n", err);
        printf("\nProtocol Terminated\n");
        return -2;
      }
      // logger->info("Initializing device: " + std::string(resourceName));
      err = TL6WL_init(resourceName, IDQuery, resetDevice, &instr);
      if (VI_SUCCESS != err) {
        sprintf_s(err_buffer, "TL6WL_init() : Error Code = %#.8lX", err);
        printf("  TL6WL_init() :\n    Error Code = %#.8lX\n", err);
        printf("\nProtocol Terminated\n");
        return -3;
      }
      // Stop any possible light
      err = TL6WL_setLED_HeadPowerStates(instr, VI_FALSE, VI_FALSE, VI_FALSE,
                                         VI_FALSE, VI_FALSE, VI_FALSE);
      err = TL6WL_setLED_HeadBrightness(instr, 0, 0, 0, 0, 0, 0);
      err = TL6WL_TU_StartStopGeneratorOutput_TU(instr, false);

      // logger->info("Reading Box Status Register.");
      printf("\nRead Box Status Register\n");

      ViUInt32 boxStatus;
      err = TL6WL_getBoxStatus(instr, &boxStatus);
      try {
        std::string boxWarning = readBoxStatusWarnings(boxStatus);
        // if length of string > 0, there was a warning
        if (!boxWarning.empty()) {
          // logger->warning(boxWarning);
          std::cerr << boxWarning << std::endl;
        } else {
          // logger->info("Box status OK.");
        }
      } catch (const std::exception& e) {
        // logger->error(e.what());
        std::cerr << e.what() << std::endl;
        return -1;
      }

      std::cout << "\n" << std::endl;

      if (VI_SUCCESS != err) {
        throw std::runtime_error(
            "Chrolispp.cpp main(): Error stopping signal generator.");
      }
    }
    while (!arduinoFound &&
           !skipArduino) {  // break if arduino is found or user skips
      std::cout << "Enter Arduino COM port number (enter n to skip arduino):";
      std::cin >> comPort;
      if (comPort == "n") {
        skipArduino = true;
        break;
      }
      //COM_PORT = getPortName(std::stoi(comPort));
      comPort = "COM" + comPort;  // prepend COM to port number
      COM_PORT = stringToWCHAR(comPort);

      std::cout << "Trying to connect to Arduino on port " << comPort << "..."
                << std::endl;
      try {
        h_Serial = createSerialHandle(COM_PORT);
        //h_Serial = openSerialHandle(COM_PORT);
      } catch (const com_init_error& e) {
        std::cerr << "Error initializing serial port: " << e.what()
                  << std::endl;
        continue;  // ask for COM port again
      }

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
      firmwareVersion = sendCommandToArduino(h_Serial, VERSION_CHECK);
      if (firmwareVersion == 4) {
        arduinoFound = true;
      } else {
        std::cerr << "Invalid firmware version from Arduino (expected 4): "
                  << +firmwareVersion << std::endl;
        return -1;
      }
    }
    if (arduinoFound) {
      std::cout << "Arduino detected with firmware ID: " << +firmwareVersion
                << std::endl;
    } else {
      std::cout << "Skipping Arduino connection." << std::endl;
    }
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
  std::string modeString = keyPressMode ? "key-press" : "protocol ";
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
    std::ostringstream oss;
    oss << "Arduino firmware version: " << +firmwareVersion;
    logger->info(oss.str());
  }
  /*
  // Minimal example defining ProtocolPlanner:
  #include <vector>

  #include "ProtocolPlanner.hpp"
  #include "ProtocolStep.hpp"
  #include "TL6WL.h"
  int main() {
    ViSession instr = 0;
    std::vector<ProtocolStep> protocolSteps;
    std::unique_ptr<Logger> logger;  // for accessing the logger outside try
    std::unique_ptr<ProtocolPlanner> planner;

    for (int i = 0; i < 10; i++) {
        ProtocolStep protocolStep =
        ProtocolStep(0, i + 10, i + 10, 5 + i, 10 * i + 5);
        protocolSteps.push_back(protocolStep);
    }

    // Create ProtocolPlanner object with instr, protocolSteps and logger
    //ProtocolPlanner planner(instr, protocolSteps, logger.get());
    planner =
      std::make_unique<ProtocolPlanner>(instr, protocolSteps, logger.get());
  }
  */
  std::unique_ptr<ProtocolPlanner> protocolPlanner;
  if (!keyPressMode) {
    logger->info("Protocol file: " + fpath);
    // Sanity checking the protocol steps
    if (protocolSteps.empty()) {
      logger->error(
          "Protocol steps are empty. Might be due to bad initialization of the "
          "protocolSteps variable.");
    }
    for (int i_step = 0; i_step < protocolSteps.size(); i_step++) {
      std::cout << "Step " << i_step + 1 << ":\n";
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
        std::cout << std::to_string(step.n_pulses) << std::endl;
        sprintf_s(err_buffer, "Invalid number of pulses in step %d: %d",
                  i_step + 1, step.n_pulses);
        logger->error(err_buffer);
        std::cerr << err_buffer << std::endl;
        return -1;
      }
      LED_ValidateBrightness(step.brightness);
      // Print step for user to review
      step.printStep();
    }
    if (arduinoFound) {
      protocolPlanner = std::make_unique<ProtocolPlanner>(
          instr, protocolSteps, logger.get(), h_Serial);
    } else {
      protocolPlanner = std::make_unique<ProtocolPlanner>(
          instr, protocolSteps, logger.get(), std::nullopt);
    }
  }
  // Log and print protocol
  std::cout << protocolPlanner->toChars("", "\t", "\t\t") << std::endl;

  logger->multiLineInfo(protocolPlanner->toChars("", "\t", "\t\t"));
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

  logger->trace("Bitness is " + std::string(bitness));
  printf("This is start of Thorlabs CHROLIS TL6WL logging output [%s]!\n\n",
         bitness);

  printf("START : Chrolis++\n");

  // Start thread to listen for escape key
  cleanupContext.h_Serial = h_Serial;
  cleanupContext.instr = instr;
  cleanupContext.logger = &logger;

  // Update the assignment to match the new type
  std::signal(SIGINT, signalHandler);
  std::optional<std::thread> arduinoThread;
  if (keyPressMode) {
    std::cout << "Press Ctrl+C to quit the key-press mode." << std::endl;
    // TODO: add listener to one button, say, S, to stim for 4 s with LED 0.
    //      If it works, add control as well.
    // TODO: pressing S multiple times creates a queue of multiple stims! Might
    // need to avoid this somehow (accidental pressing twice).
    //  Quit after one stim?
    // define sequence for LED2 and LED4

    // always start with stopping an eventually running timing unit (TU)

    err = TL6WL_TU_StartStopGeneratorOutput_TU(instr, false);

    // for a new sequence always clear the current sequence of the timing unit
    // (TU)

    err = TL6WL_TU_ResetSequence(instr);

    ViUInt8 sigNr1 = 2;               // LED2
    ViBoolean activeLow1 = 0;         // this is positive logic (not active low)
    ViUInt32 startDelayus1 = 0;       // this is start delay of 0ms
    ViUInt32 activeTimeus1 = 500000;  // this is half a second
    ViUInt32 inactiveTimeus1 = 1000000;  // this is 1s
    ViUInt32 repetitionCount1 = 3;       // number of repetitions

    ViUInt8 sigNr2 = 4;               // LED4
    ViBoolean activeLow2 = 0;         // this is positive logic (not active low)
    ViUInt32 startDelayus2 = 500000;  // this is start delay of half a second
    ViUInt32 activeTimeus2 = 500000;  // this is half a second
    ViUInt32 inactiveTimeus2 = 1000000;  // this is 1s
    ViUInt32 repetitionCount2 = 3;       // number of repetitions
    std::cout << "Setting up demo run" << std::endl;
    TL6WL_setLED_HeadBrightness(instr, 100, 100, 100, 100, 100, 100);

    TL6WL_TU_AddGeneratedSelfRunningSignal(instr, sigNr1, activeLow1,
                                           startDelayus1, activeTimeus1,
                                           inactiveTimeus1, repetitionCount1);
    TL6WL_TU_AddGeneratedSelfRunningSignal(instr, 7, activeLow1, startDelayus1,
                                           activeTimeus1, inactiveTimeus1,
                                           repetitionCount1);
    TL6WL_TU_AddGeneratedSelfRunningSignal(instr, sigNr2, activeLow2,
                                           startDelayus2, activeTimeus2,
                                           inactiveTimeus2, repetitionCount2);

    // start the sequence (this is the software trigger)
    std::cout << "Set up power states" << std::endl;
    TL6WL_setLED_HeadPowerStates(instr, VI_FALSE, VI_TRUE, VI_FALSE, VI_TRUE,
                                 VI_FALSE, VI_FALSE);

    err = TL6WL_TU_StartStopGeneratorOutput_TU(instr, true);
    if (VI_SUCCESS != err) {
      printf(
          " TL6WL_TU_StartStopGeneratorOutput_TU  :\n    Error Code = "
          "%#.8lX\n",
          err);
      printf("\nSample Terminated\n");
    } else {
      printf("Success");
    }
    Sleep(5000);  // TODO: one has to wait a proper time! Or set up a listener
                  // to the generator output?
    std::cout << "Done" << std::endl;
    while (true) {
      if (_kbhit()) {        // Check if a key was pressed
        char ch = _getch();  // Read the key (without Enter)
        if (ch == 'S' || ch == 's') {
          std::cout << "LED 0: Stimulating for 4 seconds." << std::endl;
          logger->protocol("LED 0: Stimulating for 4 seconds.");
          // LED_PulseNTimesWithArduino(instr, 0, 4000, 0, 1, 100, h_Serial,
          //                            dac_resolution_bits);
          ViUInt16 brightness = 1000;  // 100% brightness
          LED_PulseNTimes(instr, 0, 4000, 1, 1, brightness, USE_BOB);
          logger->protocol("LED 0: Done.");
        } else if (ch == 'Q' || ch == 'q') {
          // TODO: test this quit method. If works, then sigint can be limited
          // to protocol mode? Also, if this works, then q + enter in other
          // cases can be replaced.
          break;  // Exit loop
        }
      }
    }
  } else {  // Run protocol steps
    if (!protocolPlanner) {
      throw std::runtime_error("ProtocolPlanner was not properly initialized!");
    }
    protocolPlanner->setUpDevice();
    std::cout << "Press Ctrl+C to cancel the protocol." << std::endl;
    try {
      protocolPlanner->executeProtocol();
    } catch (const std::runtime_error& e) {
      std::cerr << "Runtime error during protocolPlanner::executeProtocol(): "
                << e.what() << std::endl;
      err = cleanup(instr, h_Serial, &logger);
      return -1;
    }
  }

  printf("\nClose Device\n");
  err = cleanup(instr, h_Serial, &logger);
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
