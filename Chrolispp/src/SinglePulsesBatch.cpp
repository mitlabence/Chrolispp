#include "SinglePulsesBatch.hpp"

#include "TL6WL.h"
#include "Timing.hpp"
#include "constants.hpp"
SinglePulsesBatch::SinglePulsesBatch(ViSession instr,
                                     const std::vector<ProtocolStep>& steps,
                                     Logger* logger_ptr)
    : ProtocolBatch(instr, steps, logger_ptr) {
  if (steps.empty()) {
    throw std::invalid_argument("No protocol steps provided.");
  }
  // Calculate busy and total duration. Busy duration is total duration minus
  // the very last idle time
  int busy_ms = 0;
  int total_ms = 0;
  for (const auto& step : steps) {
    if (step.n_pulses != 1) {
      throw std::invalid_argument(
          "SinglePulsesBatch can only contain steps with a single pulse.");
    }
    // Set LED mask proper digit to 1, e.g. for led_index = 2, led_mask =
    // 0b000100
    int led_index = step.led_index;
    led_mask |= (1 << led_index);
    total_ms += step.getTotalDurationMs();
  }
  // subtract the last idle time (time between pulses of the last step) to get
  // busy_ms
  busy_ms = total_ms - steps.back().time_between_pulses_ms;
  busy_duration_ms = std::chrono::milliseconds(busy_ms);
  total_duration_ms = std::chrono::milliseconds(total_ms);

  protocol_steps = steps;
}
std::chrono::milliseconds SinglePulsesBatch::getBusyDurationMs() const {
  return busy_duration_ms;
}

std::chrono::milliseconds SinglePulsesBatch::getTotalDurationMs() const {
  return total_duration_ms;
}

std::chrono::microseconds SinglePulsesBatch::execute() {
  logger_ptr->trace("SinglePulsesBatch execute()");
  if (execute_attempted) {
    throw std::logic_error(
        "SinglePulsesBatch: attempting to execute already executed batch.");
  }
  // Loop over steps, set up LED power states and brightness for each step.
  auto start = std::chrono::high_resolution_clock::now();
  ViStatus err;
  ViBoolean led_states[6] = {VI_FALSE, VI_FALSE, VI_FALSE,
                             VI_FALSE, VI_FALSE, VI_FALSE};
  int led_brightness[6] = {0, 0, 0, 0, 0, 0};
  int brightness = protocol_steps[0].brightness;
  int led_index = protocol_steps[0].led_index;
  // Set up LED head brightness values
  led_brightness[led_index] = brightness;
  int i_step = 0;
  execute_attempted = true;
  for (auto& step : protocol_steps) {
    char* chs = step.toChars("");
    logger_ptr->protocol(std::string("Executing step #") +
                         std::to_string(i_step + 1) + " in batch: " + chs);
    // Step 1 brightnesses have already been set up when setUpThisBatch() was
    // called
    if (i_step > 0) {
      // Turn off previous LED
      led_states[led_index] = VI_FALSE;
      led_brightness[led_index] = 0;
      // Set up current LED
      led_index = step.led_index;
      brightness = step.brightness;
      led_states[led_index] = VI_TRUE;
      led_brightness[led_index] = brightness;
      // Set up LED head brightnesses
      logger_ptr->trace(
          "SinglePulsesBatch::execute(): TL6WL_setLED_HeadBrightness() to turn "
          "on LED " +
          std::to_string(led_index));
      err = TL6WL_setLED_HeadBrightness(
          instr, led_brightness[0], led_brightness[1], led_brightness[2],
          led_brightness[3], led_brightness[4], led_brightness[5]);
      if (VI_SUCCESS != err) {
        logger_ptr->error(
            "SinglePulsesBatch::execute(): Error setting LED head brightness.");
        throw std::runtime_error(
            "SinglePulsesBatch::execute(): Error setting LED head brightness.");
      }
    } else {  // first step, brightness has been set up already
      led_index = step.led_index;
      led_states[led_index] = VI_TRUE;
    }
    // Set up LED head states
    logger_ptr->trace(
        "SinglePulsesBatch::execute(): TL6WL_setLED_HeadPowerStates() to power "
        "on LED " +
        std::to_string(led_index));
    err = TL6WL_setLED_HeadPowerStates(instr, led_states[0], led_states[1],
                                       led_states[2], led_states[3],
                                       led_states[4], led_states[5]);

    if (VI_SUCCESS != err) {
      logger_ptr->error(
          "SinglePulsesBatch::execute(): Error setting LED head power states.");
      throw std::runtime_error(
          "SinglePulsesBatch::execute(): Error setting LED head power states.");
    }
    Timing::precise_sleep_for(std::chrono::milliseconds(step.pulse_width_ms));
    int break_duration_ms = step.time_between_pulses_ms;
    if (break_duration_ms > 0) {  // if there is a break, need to turn off LED
      // Turn off current LED
      led_states[led_index] = VI_FALSE;
      led_brightness[led_index] = 0;
      logger_ptr->trace(
          "SinglePulsesBatch::execute(): Trying to shut down head power "
          "states...");
      err = TL6WL_setLED_HeadPowerStates(instr, VI_FALSE, VI_FALSE, VI_FALSE,
                                         VI_FALSE, VI_FALSE, VI_FALSE);
      if (VI_SUCCESS != err) {
        logger_ptr->error(
            "SinglePulsesBatch::execute(): Error setting LED head power "
            "states.");
        throw std::runtime_error(
            "SinglePulsesBatch::execute(): Error setting LED head power "
            "states.");
      }
      Timing::precise_sleep_for(std::chrono::milliseconds(break_duration_ms));
    }
    i_step++;
  }
  err = TL6WL_setLED_HeadPowerStates(instr, VI_FALSE, VI_FALSE, VI_FALSE,
                                     VI_FALSE, VI_FALSE, VI_FALSE);
  if (VI_SUCCESS != err) {
    throw std::runtime_error(
        "SinglePulsesBatch::execute(): Error turning off LEDs after pulse "
        "chain.");
  }
  err = TL6WL_setLED_HeadBrightness(instr, 0, 0, 0, 0, 0, 0);
  if (VI_SUCCESS != err) {
    throw std::runtime_error(
        "SinglePulsesBatch::execute(): Error setting LED head brightness to 0 "
        "after pulse chain.");
  }
  err = TL6WL_TU_StartStopGeneratorOutput_TU(instr, false);
  if (VI_SUCCESS != err) {
    throw std::runtime_error(
        "SinglePulsesBatch::execute(): Error stopping signal generator.");
  }
  logger_ptr->trace("SinglePulsesBatch::execute() done.");
  auto end = std::chrono::high_resolution_clock::now();
  auto actual_duration_us =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  busy_duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      actual_duration_us);  // update actual busy duration
  return actual_duration_us;
}

void SinglePulsesBatch::setUpNextBatch(ProtocolBatch& next_batch) {
  logger_ptr->trace("SinglePulsesBatch setUpNextBatch()");
  // TODO: avoid repeating this code in other implementations of ProtocolBatch
  if (!execute_attempted) {
    throw std::logic_error(
        "Cannot set up next batch before executing this batch.");
  }

  auto start = std::chrono::high_resolution_clock::now();
  next_batch.setUpThisBatch();
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::milliseconds duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          end - start);  // Truncate to milliseconds
  // Wait for rest of the (busy - total duration) time if any left after
  // <duration> time passed Convert duration to nearest milliseconds int
  if (duration < total_duration_ms - busy_duration_ms) {
    Timing::precise_sleep_for(total_duration_ms - busy_duration_ms - duration);
  }
  logger_ptr->trace("SinglePulsesBatch setUpNextBatch() done.");
}

void SinglePulsesBatch::setUpThisBatch() {
  logger_ptr->trace("SinglePulsesBatch setUpThisBatch()");
  int brightness = protocol_steps[0].brightness;
  int led_index = protocol_steps[0].led_index;
  // Set up LED head brightness values
  int led_brightness[6] = {0, 0, 0, 0, 0, 0};
  led_brightness[led_index] = brightness;
  ViStatus err = TL6WL_setLED_HeadBrightness(
      instr, led_brightness[0], led_brightness[1], led_brightness[2],
      led_brightness[3], led_brightness[4], led_brightness[5]);
  if (VI_SUCCESS != err) {
    throw std::runtime_error(
        "SinglePulsesBatch::setUpThisBatch(): Error setting LED head "
        "brightness.");
  }
  logger_ptr->trace("SinglePulsesBatch setUpThisBatch() done.");
}

char* SinglePulsesBatch::toChars(const std::string& prefix,
                                 const std::string& step_level_prefix) {
  return ProtocolBatch::batchToChars("SinglePulsesBatch", prefix,
                                     step_level_prefix);
}