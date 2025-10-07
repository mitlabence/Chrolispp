#include "ProtocolPlanner.hpp"

#include <cmath>
#include <format>
#include <iostream>
#include <stdexcept>
#include <string_view>

#include "InitialBreakBatch.hpp"
#include "LEDFunctions.hpp"
#include "Logger.hpp"
#include "PulseChainBatch.hpp"
#include "SinglePulsesBatch.hpp"
#include "TL6WL.h"
#include "Timing.hpp"
#include "constants.hpp"
static void logError(Logger& logger_ptr, std::string_view func_name,
                     uint32_t err, bool verbose = true) {
  std::string err_msg =
      std::format("{}(): Error Code = {:#08X}", func_name, err);
  logger_ptr.error(err_msg);
  if (verbose) {
    // also print the error code
    std::cout << err_msg << std::endl;
  }
}
// Constructor implementation
ProtocolPlanner::ProtocolPlanner(ViSession instr,
                                 std::vector<ProtocolStep> protocolSteps,
                                 Logger* logger_ptr)
    : instr(instr), steps(std::move(protocolSteps)), logger_ptr(logger_ptr) {
  // TODO: for each different wavelength, one can already program the LED
  // machine with calculated delays. If multiple steps with the same
  // wavelength
  // exist, these might be set up as different steps with delays in the
  // beginning (test this), or one has to be smart and do it with sleep()
  // and
  // reprogramming the LED machine in between.
  if (steps.empty()) {
    logger_ptr->error("No protocol steps provided.");
    throw std::invalid_argument("No protocol steps provided.");
  }
  // loop over all steps
  int i_step = 1;  // for user display, start with 1
  for (auto& step : steps) {
    int validation_result = validateStep(step);
    switch (validation_result) {
      case 0:  // valid step
        break;
      case 1: {
        std::string err_msg =
            "Invalid LED index in step " + std::to_string(i_step);
        logger_ptr->error(err_msg);
        throw std::invalid_argument(err_msg);
      }
      case 2: {
        std::string err_msg = "Invalid step " + std::to_string(i_step) +
                              ": 0 pulses and "
                              "pulse duration != 0, not a valid step.";
        logger_ptr->error(err_msg);
        throw std::invalid_argument(err_msg);
      }
      case 3: {
        std::string err_msg = "Invalid brightness in step " +
                              std::to_string(i_step) + ": corrected to 100%.";
        logger_ptr->warning(err_msg);
        throw std::invalid_argument(err_msg);
      }
      default: {
        std::string err_msg = "Unknown validation result in step " +
                              std::to_string(i_step) + ": " +
                              std::to_string(validation_result);
        logger_ptr->error(err_msg);
        throw std::invalid_argument(err_msg);
      }
    }
    i_step++;
  }
  // Merge compatible steps to remove redundant steps
  mergeSteps(steps);
  n_steps = static_cast<size_t>(steps.size());
  // Produce batches (groups of steps that can be programmed at once) from list
  // of steps
  batches = translateToBatches();
  if (batches.empty()) {
    throw std::runtime_error("No batches created from protocol steps.");
  }
  batches_loaded = true;
}

/*
Check if two steps (it is assumed that step2 follows step1 directly) are
compatible and return:
- 0 if not compatible,
- 1 if both are breaks (0
brightness) i.e. compatible,
- 2 if both compatible non-breaks: both have same
led index, brightness, pulse width, time between pulses
- 3 if step1 is a gapless single pulse (i.e. time between pulses = 0 ms) and
  step2 is a break (0 brightness) or a compatible single pulse
    (same led_index, brightness; n_pulses=1). In these cases, the break/time
between pulses is the time between pulses of the merged segment, and the pulse
width is the sum of the two pulse widths.
*/
static int stepsCompatibleForMerge(const ProtocolStep& step1,
                                   const ProtocolStep& step2) {
  if (step1.isBreak() && step2.isBreak()) {
    return 1;
  }
  if ((step1.led_index == step2.led_index) &&
      (step1.brightness == step2.brightness) &&
      (step1.pulse_width_ms == step2.pulse_width_ms) &&
      (step1.time_between_pulses_ms == step2.time_between_pulses_ms)) {
    return 2;
  }
  if (step1.isGaplessSinglePulse()) {
    if (step2.isBreak()) {
      return 3;
    } else if ((step1.led_index == step2.led_index) &&
               (step1.brightness == step2.brightness) &&
               (step2.n_pulses == 1)) {
      return 4;
    }
  }
  return 0;
}

/*Merge possible consecutive breaks or consecutive compatible steps (same
 * led index, brightness, pulse width, time between pulses)*/
void ProtocolPlanner::mergeSteps(std::vector<ProtocolStep>& protocolSteps) {
  size_t i = 0;
  while (i < protocolSteps.size() - 1) {
    ProtocolStep& current = protocolSteps[i];
    ProtocolStep& next = protocolSteps[i + 1];
    int compatibility_value = stepsCompatibleForMerge(current, next);
    switch (compatibility_value) {
      case 0: {
        // Not compatible, do nothing
        i++;
        continue;
      }
      case 1: {
        // Both are breaks, merge into current, erase next
        ViInt32 current_break = current.getBreakDurationMs();
        ViInt32 next_break = next.getBreakDurationMs();
        current.setBreakDuration(current_break + next_break);
        std::string merge_msg =
            std::format("Merge consecutive breaks: {} ms + {} ms = {} ms",
                        current_break, next_break, current_break + next_break);
        logger_ptr->trace(merge_msg);
        std::cout << merge_msg << std::endl;
        protocolSteps.erase(protocolSteps.begin() + i + 1);
        // Don't increment i but check again with same current in case of
        // multiple matches
        continue;
      }
      case 2: {
        std::string merge_msg = std::format(
            "Merge compatible steps: LED {}, brightness {}, pulse width {} "
            "ms, time between pulses {} ms, number of pulses {} + {} = {}",
            current.led_index, current.brightness, current.pulse_width_ms,
            current.time_between_pulses_ms, current.n_pulses, next.n_pulses,
            current.n_pulses + next.n_pulses);
        logger_ptr->trace(merge_msg);
        std::cout << merge_msg << std::endl;
        // Both are compatible non-breaks, merge into current, erase next
        current.n_pulses += next.n_pulses;
        protocolSteps.erase(protocolSteps.begin() + i + 1);
        // Don't increment i but check again with same current in case of
        // multiple matches
        continue;
      }
      case 3: {
        // current is gapless single pulse, next is break or compatible single
        // pulse, merge into current, erase next
        ViInt32 next_break_duration;
        if (next.isBreak()) {
          next_break_duration = next.getBreakDurationMs();
        } else {
          next_break_duration = next.time_between_pulses_ms;
        }
        std::string merge_msg = std::format(
            "Merge gapless single pulse with {}: LED {}, brightness {}, "
            "new pulse width {} ms, new time between pulses {} ms",
            next.isBreak() ? "break" : "compatible single pulse",
            current.led_index, current.brightness,
            current.pulse_width_ms + next.pulse_width_ms,
            current.time_between_pulses_ms + next_break_duration);
        logger_ptr->trace(merge_msg);
        std::cout << merge_msg << std::endl;
        current.pulse_width_ms += next.pulse_width_ms;  // +0 if next is break
        current.time_between_pulses_ms += next_break_duration;
        protocolSteps.erase(protocolSteps.begin() + i + 1);
        // Don't increment i but check again with same current in case of
        // multiple matches
        continue;
      }
      default: {  // Should not happen
        throw std::logic_error("Unexpected compatibility value: " +
                               std::to_string(compatibility_value));
      }
    }
  }
}
/*
Given the list of steps, translate it into a sequence of batches (groups of
steps that can be programmed at once, definition in ProtocolBatch.hpp).
*/
std::vector<std::unique_ptr<ProtocolBatch>>
ProtocolPlanner::translateToBatches() {
  if (steps.empty()) {  // return empty vector if no steps
    return std::vector<std::unique_ptr<ProtocolBatch>>();
  }
  // Start with first step in steps as beginning of batch 1.
  // Loop over subsequent steps until a step is not compatible with the rest
  // in the definition of a batch.
  std::vector<std::unique_ptr<ProtocolBatch>> batches;
  int step_cursor = 0;
  while (step_cursor < n_steps) {
    std::unique_ptr<ProtocolBatch> next_batch = getNextBatch(step_cursor);
    batches.push_back(std::move(next_batch));
  }
  return batches;
}

/*
Check if the step parameters are valid.
Returns the following:
0: valid step
1: LED index invalid
2: n_pulses == 0 and not a break (i.e. pulse_width_ms != 0)
3: brightness invalid (auto-clipped to 1000), but step is made valid
Checks:
- LED index in range 0-5
- n_pulses > 0
- brightness (non-negative number) in range 0-1000. If >1000, set to 1000
and show a warning (not logged).
*/
ValidationResult ProtocolPlanner::validateStep(ProtocolStep& step) {
  if (!LED_ValidateLEDIndex(step.led_index)) {
    return INVALID_LED_INDEX;
  }
  if (step.n_pulses == 0 &&
      step.brightness != 0) {  // if 0 pulses (and not a break, i.e. brightness
                               // != 0), invalid step
    return INVALID_PULSE_COUNT;
  }
  if (!LED_ValidateBrightness(step.brightness)) {
    return INVALID_BRIGHTNESS;
  }
  return VALID_STEP;
}

/*
Given the current state of the class, assuming the current batch starts with
the class variable current_step_index, find the whole current batch and return
it. See batch definition in ProtocolBatch.hpp.
*/
std::unique_ptr<ProtocolBatch> ProtocolPlanner::getNextBatch(int& step_cursor) {
  bool next_batch_found = false;
  bool initial_break_type = false;
  bool single_pulse_batch_type =
      false;  // Separate batching logic based on which type of batch: chained
              // pulses or single pulses.
  int i_current_candidate =
      step_cursor;  // start with first step of current batch
  int batch_end = -1;
  int led_mask = 0b000000;  // to track which LEDs are used in current batch for
                            // pulse chains
  // loop over steps until end of current batch found or end of steps
  while (!next_batch_found && (i_current_candidate < n_steps)) {
    ProtocolStep& current = steps[i_current_candidate];
    // If current step is break: end current batch with this break
    if (current.isBreak()) {
      if (i_current_candidate == step_cursor) {
        // If this is first step of current batch, set batch type to initial
        // break
        initial_break_type = true;
      }
      batch_end = i_current_candidate;
      break;  // Quit while loop
    }
    // If current step is single pulse: include each consecutive step that is
    // also single pulse.
    if (current.n_pulses == 1) {
      // If this is first step of current batch, set batch type to single
      if (i_current_candidate == step_cursor) {
        single_pulse_batch_type = true;
      } else {
        // If not first step of current batch, check if batch type is single
        // pulse. If not, then current step (a single pulse) is start of next
        // batch
        if (!single_pulse_batch_type) {
          batch_end = i_current_candidate - 1;  // end of current batch found
          break;
        }
      }
    } else {
      // Current step is a chain of pulses (n_pulses > 1).
      if (i_current_candidate == step_cursor) {
        // If first step of current batch, no flags to set, only led mask
        led_mask |= (1 << current.led_index);
      } else {
        // If not first step of current batch, check if batch type is chained
        // pulses. If not, then current step (a chain of pulses) is start of
        // next batch
        if (single_pulse_batch_type) {
          batch_end = i_current_candidate - 1;  // end of current batch found
          break;
        } else {
          // If batch type is chained pulses, check if LED index is already
          // used in this batch
          if (led_mask & (1 << current.led_index)) {
            // LED already used in this batch, so end of current batch
            batch_end = i_current_candidate - 1;
            break;
          } else {
            // LED not yet used in this batch, add to mask and continue
            led_mask |= (1 << current.led_index);
          }
        }
      }
    }
    i_current_candidate++;
  }
  // Create batch object with correct type
  // Take (unique pointers to) subset of steps from current_step_index to
  // batch_end (inclusive) if batch_end == -1, change it to last element
  if (batch_end == -1) {
    batch_end = n_steps - 1;
  }
  // Take reference to subset of steps from current_step_index to batch_end
  // (inclusive)
  std::vector<ProtocolStep> batch_steps;
  batch_steps.insert(batch_steps.end(), steps.begin() + step_cursor,
                     steps.begin() + batch_end + 1);
  // Update current_step_index
  if (batch_end != -1) {
    step_cursor = batch_end + 1;  // start of next batch.
  } else {
    step_cursor = n_steps;  // end of steps reached
    // return std::make_unique();
  }
  // Create and return batch object
  if (initial_break_type) {
    return std::make_unique<InitialBreakBatch>(instr, std::move(batch_steps),
                                               logger_ptr);
  }
  if (single_pulse_batch_type) {
    return std::make_unique<SinglePulsesBatch>(instr, std::move(batch_steps),
                                               logger_ptr);
  } else {
    return std::make_unique<PulseChainBatch>(instr, std::move(batch_steps),
                                             logger_ptr);
  }
}

void ProtocolPlanner::setUpDevice() {
  ViStatus err;
  std::string err_msg;
  try {
    // Set master brightness to 100%
    err = TL6WL_setLED_LinearModeValue(instr, 1000);
    if (VI_SUCCESS != err) {  // Handle failure to set master brightness
      logError(*logger_ptr, "TL6WL_setLED_LinearModeValue", err);
      err = TL6WL_close(instr);
      if (VI_SUCCESS != err) {
        logError(*logger_ptr, "TL6WL_close", err);
        throw std::runtime_error(
            "Fatal error: closing device communication failed");
      }
    } else {
      logger_ptr->trace("TL6WL_setLED_LinearModeValue() successful");
    }
    // Set all LED brightness to 0%
    err = TL6WL_setLED_HeadBrightness(instr, 0, 0, 0, 0, 0, 0);
    if (VI_SUCCESS != err) {
      logError(*logger_ptr, "TL6WL_setLED_HeadBrightness", err);
      err = TL6WL_close(instr);
      if (VI_SUCCESS != err) {
        logError(*logger_ptr, "TL6WL_close", err);
        throw std::runtime_error(
            "Fatal error: closing device communication failed");
      }
    } else {
      logger_ptr->trace("TL6WL_setLED_HeadBrightness() successful");
    }
  } catch (const std::exception& e) {
    const char* err_str = e.what();
    if (err_str == nullptr) {
      err_str = "Unknown error";
      return;
    }
  }
  device_set_up = true;
  logger_ptr->info("Device set up successfully.");
}
/*
Execute the protocol (all batches). This assumes that the device has been set
up.
*/
void ProtocolPlanner::executeProtocol() {
  if (!batches_loaded) {
    throw std::runtime_error("Batches were not loaded. ");
  }
  if (!device_set_up) {
    throw std::runtime_error(
        "Device not set up. Call setUpDevice() before execute().");
  }
  try {
    if (batches.size() == 1) {
      ProtocolBatch& batch = *batches[0];
      batch.setUpThisBatch();
      batch.execute();
      batches_loaded = false;
      // If total duration > busy duration, sleep for (total - busy)
      std::chrono::milliseconds total_duration_ms = batch.getTotalDurationMs();
      std::chrono::milliseconds busy_duration_ms = batch.getBusyDurationMs();
      if (total_duration_ms > busy_duration_ms) {
        logger_ptr->trace(
            "Sleeping for remaining time: " +
            std::to_string((total_duration_ms - busy_duration_ms).count()) +
            " ms");
        Timing::precise_sleep_for(total_duration_ms - busy_duration_ms);
      }
    } else if (batches.size() > 1) {  // TODO: batches cannot be empty
      // Set up first batch
      ProtocolBatch& current_batch = *batches[0];
      ProtocolBatch& next_batch = *batches[1];
      current_batch.setUpThisBatch();
      batches_loaded = false;  // Block from restarting
      // *** Time critical part starts here ***
      // Execute first batch
      current_batch.execute();
      //
      for (size_t i_batch = 0; i_batch + 1 < batches.size(); ++i_batch) {
        std::cout << "Executing batch " << i_batch << " of " << batches.size()
                  << std::endl;
        current_batch = *batches[i_batch];
        next_batch = *batches[i_batch + 1];
        std::cout << "Current batch: " << current_batch.toChars("", "\t")
                  << std::endl;
        std::cout << "Next batch: " << next_batch.toChars("", "\t")
                  << std::endl;
        // Set up next batch
        current_batch.setUpNextBatch(next_batch);
        // Execute next batch
        next_batch.execute();
      }
      // Sleep for (total - busy) duration of last step
      // if total duration > busy duration
      std::chrono::milliseconds total_duration_ms =
          next_batch.getTotalDurationMs();
      std::chrono::milliseconds busy_duration_ms =
          next_batch.getBusyDurationMs();
      if (total_duration_ms > busy_duration_ms) {
        logger_ptr->trace(
            "Sleeping for remaining time: " +
            std::to_string((total_duration_ms - busy_duration_ms).count()) +
            " ms");
        Timing::precise_sleep_for(total_duration_ms - busy_duration_ms);
      }
    }
  } catch (const std::exception& e) {
    shutDownDevice();
    const char* err_str = e.what();
    if (err_str == nullptr) {
      err_str = "Unknown error";
    }
    throw std::runtime_error(err_str);
  }
  shutDownDevice();
}
void ProtocolPlanner::shutDownDevice() {
  ViStatus err;
  std::string err_msg;
  try {
    // Turn off device
    logger_ptr->trace("shutDownDevice()");
    err = TL6WL_setLED_LinearModeValue(instr, 0);
    if (VI_SUCCESS != err) {  // Handle failure to turn off device
      logError(*logger_ptr, "TL6WL_setLED_LinearModeValue", err);
    } else {
      logger_ptr->trace("TL6WL_setLED_LinearModeValue() successful");
    }
    err = TL6WL_setLED_HeadPowerStates(instr, VI_FALSE, VI_FALSE, VI_FALSE,
                                       VI_FALSE, VI_FALSE, VI_FALSE);
    if (VI_SUCCESS != err) {  // Handle failure to turn off LEDs
      logError(*logger_ptr, "TL6WL_setLED_HeadPowerStates", err);
    } else {
      logger_ptr->trace("TL6WL_setLED_HeadPowerStates() successful");
    }
    err = TL6WL_close(instr);
    if (VI_SUCCESS != err) {  // Handle failure to close device communication
      logError(*logger_ptr, "TL6WL_close", err);
      throw std::runtime_error(
          "Fatal error: closing device communication failed");
    } else {
      logger_ptr->trace("TL6WL_close() successful");
    }
    device_set_up = false;
    logger_ptr->trace("shutDownDevice() done.");
  } catch (const std::exception& e) {
    const char* err_str = e.what();
    if (err_str == nullptr) {
      err_str = "Unknown error";
    }
    throw std::runtime_error(err_str);
  }
}

/*
Return the protocol planner as a string (char array). The caller is responsible
for deleting the returned char array.
prefix: prefix for the first line (describing the whole protocol), e.g. "\t"
batch_level_prefix: prefix for each batch line only, e.g. "\t\t"
step_level_prefix: prefix for each step level line, e.g. "\t\t\t"
*/
char* ProtocolPlanner::toChars(const std::string& prefix,
                               const std::string& batch_level_prefix,
                               const std::string& step_level_prefix) {
  // Get all the toChars() functions of the constituent batches, return single
  // chars separated by \n
  // TODO: implement the toChars() functions, add a (global constant)
  // BATCH_TOCHARS_BUFFERSIZE. Then use this in the implementations, and here
  // define the bufferSize accordingly as a multiple
  //
  // Calculate the required buffer size
  const int bufferSize =
      Constants::PROTOCOL_PLANNER_HEADER_CHARS_BUFFERSIZE + prefix.length() +
      batches.size() * (Constants::BATCH_HEADER_CHARS_BUFFERSIZE +
                        batch_level_prefix.length()) +
      steps.size() *
          (Constants::STEP_CHARS_BUFFERSIZE +
           step_level_prefix
               .length());  // +1 \t for batch level, +2 for \t\t at step level
  char* pPlannerChars = new char[bufferSize];

  std::snprintf(pPlannerChars, bufferSize,
                "%sProtocol with %zu batch(es), %zu step(s):\n", prefix.c_str(),
                batches.size(), n_steps);

  // Loop over batches, use their toChars() functions
  for (auto& batch : batches) {
    char* batchChars = batch->toChars(batch_level_prefix, step_level_prefix);
    // Use strncat_s if available, otherwise use strncat with bounds checking
#ifdef _MSC_VER  // Check if compiling with Microsoft Visual Studio
    strncat_s(pPlannerChars, bufferSize, batchChars,
              bufferSize - std::strlen(pPlannerChars) - 1);
    strncat_s(pPlannerChars, bufferSize, "\n",
              bufferSize - std::strlen(pPlannerChars) - 1);
#else
    strncat(plannerChars, batchChars,
            bufferSize - std::strlen(plannerChars) - 1);
    strncat(plannerChars, "\n", bufferSize - std::strlen(plannerChars) - 1);
#endif
    delete[] batchChars;
  }
  return pPlannerChars;
}