#include "PulseChainBatch.hpp"

#include "Timing.hpp"
#include "constants.hpp"

PulseChainBatch::PulseChainBatch(unsigned short batch_id, ViSession instr,
                                 const std::vector<ProtocolStep>& steps,
                                 Logger* logger_ptr)
    : ProtocolBatch(batch_id, instr, steps, logger_ptr) {
  if (steps.empty()) {
    throw std::invalid_argument("No protocol steps provided.");
  }
  // Calculate busy and total duration. Busy duration is total duration minus
  // the very last idle time
  int busy_us = 0; // TODO: if at least one step has us_mode, busy_ms and total_ms should be in ns?
  int total_us = 0;
  for (const auto& step : steps) {
    // Set LED mask proper digit to 1, e.g. for led_index = 2, led_mask =
    // 0b000100
    int led_index = step.led_index;
    if (!step.isBreak()) {
      led_mask |= (1 << led_index);
    }
    total_us += step.getTotalDurationUs();
  }
  // subtract the last idle time (time between pulses of the last step last
  // pulse) to get busy_ms
  ViUInt32 last_break_duration = steps.back().time_between_pulses_us;
  has_trailing_break = last_break_duration > 0;
  busy_us = total_us - last_break_duration;

  busy_duration_us = std::chrono::microseconds(busy_us);
  total_duration_us = std::chrono::microseconds(total_us);

  protocol_steps = steps;
  batch_type = "PulseChainBatch";
}

std::chrono::microseconds PulseChainBatch::getBusyDurationUs() const {
  return busy_duration_us;
}

std::chrono::microseconds PulseChainBatch::getTotalDurationUs() const {
  return total_duration_us;
}

std::chrono::microseconds PulseChainBatch::execute() {
  logger_ptr->trace("PulseChainBatch execute()");
  if (execute_attempted) {
    throw std::logic_error(
        "PulseChainBatch: attempting to execute already executed batch.");
  }
  auto start = std::chrono::high_resolution_clock::now();
  ViStatus err;
  // Start the timer
  logger_ptr->protocol("Executing PulseChainBatch with steps:");
  logger_ptr->multiLineProtocol(toChars("\t", "\t\t"));
  ViBoolean led_states[6] = {VI_FALSE, VI_FALSE, VI_FALSE,
                             VI_FALSE, VI_FALSE, VI_FALSE};
  execute_attempted = true;
  // Go through LED mask, set led_states accordingly
  for (int i = 0; i < 6; i++) {
    if (led_mask & (1 << i)) {
      led_states[i] = VI_TRUE;
    }
  }
  logger_ptr->trace(
      "PulseChainBatch::execute(): TL6WL_setLED_HeadPowerStates() to set "
      "states to " +
      std::to_string(led_states[0]) + ", " + std::to_string(led_states[1]) +
      ", " + std::to_string(led_states[2]) + ", " +
      std::to_string(led_states[3]) + ", " + std::to_string(led_states[4]) +
      ", " + std::to_string(led_states[5]));
  err = TL6WL_setLED_HeadPowerStates(instr, led_states[0], led_states[1],
                                     led_states[2], led_states[3],
                                     led_states[4], led_states[5]);
  logger_ptr->trace(
      "PulseChainBatch::execute(): TL6WL_TU_StartStopGeneratorOutput_TU(instr, "
      "true)");
  err = TL6WL_TU_StartStopGeneratorOutput_TU(instr, true);
  if (VI_SUCCESS != err) {
    throw std::runtime_error(
        "PulseChainBatch::execute(): Error starting signal generator.");
  }

  // Cast to next millisecond
  std::chrono::milliseconds busy_duration_ms = duration_cast<std::chrono::milliseconds>(busy_duration_us + std::chrono::microseconds(999));

  Timing::precise_sleep_for(busy_duration_ms);
  logger_ptr->trace("PulseChainBatch execute() done.");
  if (has_trailing_break) {  // turn off LEDs to make sure set up of next batch
                             // does not affect light output
    err = TL6WL_setLED_HeadPowerStates(instr, VI_FALSE, VI_FALSE, VI_FALSE,
                                       VI_FALSE, VI_FALSE, VI_FALSE);
    if (VI_SUCCESS != err) {
      throw std::runtime_error(
          "PulseChainBatch::execute(): Error turning off head power states.");
    }
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto actual_duration_us =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  busy_duration_us = std::chrono::duration_cast<std::chrono::microseconds>(
      actual_duration_us);  // update actual busy duration
  return actual_duration_us;
}

void PulseChainBatch::setUpNextBatch(ProtocolBatch& next_batch) {
  auto start = std::chrono::high_resolution_clock::now();
  logger_ptr->trace("PulseChainBatch setUpNextBatch()");
  // TODO: avoid repeating this code in other implementations of ProtocolBatch
  if (!execute_attempted) {
    throw std::logic_error(
        "Cannot set up next batch before executing this batch.");
  }
  next_batch.setUpThisBatch();
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::milliseconds duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          end - start);  // Truncate to milliseconds
  // Wait for rest of the (busy - total duration) time if any left after
  // <duration> time passed Convert duration to nearest milliseconds int
  if (duration < total_duration_us - busy_duration_us) {
	  // Cast to next millisecond
	  std::chrono::milliseconds total_duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(total_duration_us - busy_duration_us - duration + std::chrono::microseconds(999));
    Timing::precise_sleep_for(total_duration_ms);
  }
  logger_ptr->trace("PulseChainBatch setUpNextBatch() done.");
}

void PulseChainBatch::setUpThisBatch() {
  logger_ptr->trace("PulseChainBatch setUpThisBatch()");
  // Loop over steps, set up periodic signal and keep track of delay times
  ViStatus err;
  // Stop timer
  // TODO: this should not be necessary, as timer is stopped in beginning of the
  // program, and at the end of each execute().
  err = TL6WL_TU_StartStopGeneratorOutput_TU(instr, false);
  if (VI_SUCCESS != err) {
    throw std::runtime_error(
        "PulseChainBatch::setUpThisBatch(): Error stopping signal generator.");
  }
  // Reset timer
  err = TL6WL_TU_ResetSequence(instr);
  if (VI_SUCCESS != err) {
    throw std::runtime_error(
        "PulseChainBatch::setUpThisBatch(): Error resetting signal generator.");
  }
  ViUInt32 duration_so_far_us = 0;
  int led_brightness[6] = {0, 0, 0, 0, 0, 0};
  // TODO: extract this logic into constructor: create an array for each
  // parameter TL6WL_TU_AddGeneratedSelfRunningSignal takes, and pre-calculate
  // the values, so here one must only loop through an index, and call
  // TL6WL_TU_AddGeneratedSelfRunningSignal two times (for LED head and BOB) per
  // index
  for (const auto& step : protocol_steps) {
    int led_index = step.led_index;
    int brightness = step.brightness;
    led_brightness[led_index] = brightness;
    if (step.isBreak()) {
        duration_so_far_us += step.getTotalDurationUs();
        continue;
    }
    err = TL6WL_TU_AddGeneratedSelfRunningSignal(
        instr, led_index + 1, VI_FALSE, duration_so_far_us,
        step.pulse_width_us, step.time_between_pulses_us,
        step.n_pulses);
    if (VI_SUCCESS != err) {
      throw std::runtime_error(
          "PulseChainBatch::setUpThisBatch(): Error adding signal to signal "
          "generator.");
      logger_ptr->error(
          "PulseChainBatch::setUpThisBatch(): Error setting up step: signalNr" +
          std::to_string(led_index + 1) + ", startDelay: " +
          std::to_string(duration_so_far_us) + "us, activeTimeus" +
          std::to_string(step.pulse_width_us) + " us, inactiveTimeus " +
          std::to_string(step.time_between_pulses_us) +
          " us, repetitionCount " + std::to_string(step.n_pulses));
    }
    // Add breakout box signal as well
    err = TL6WL_TU_AddGeneratedSelfRunningSignal(
        instr, step.led_index + 1 + 6, VI_FALSE, duration_so_far_us,
        step.pulse_width_us, step.time_between_pulses_us,
        step.n_pulses);
    if (VI_SUCCESS != err) {
      throw std::runtime_error(
          "PulseChainBatch::setUpThisBatch(): Error adding signal to signal "
          "generator.");
      logger_ptr->error(
          "PulseChainBatch::setUpThisBatch(): Error setting up (breakout "
          "board) "
          "step: signalNr" +
          std::to_string(led_index + 1 + 6) + ", startDelay: " +
          std::to_string(duration_so_far_us) + "us, activeTimeus" +
          std::to_string(step.pulse_width_us) + " us, inactiveTimeus " +
          std::to_string(step.time_between_pulses_us) +
          " us, repetitionCount " + std::to_string(step.n_pulses));
    }
    duration_so_far_us += step.getTotalDurationUs();
  }
  logger_ptr->trace(
      "PulseChainBatch::setUpThisBatch(): Setting brightnesses to " +
      std::to_string(led_brightness[0]) + ", " +
      std::to_string(led_brightness[1]) + ", " +
      std::to_string(led_brightness[2]) + ", " +
      std::to_string(led_brightness[3]) + ", " +
      std::to_string(led_brightness[4]) + ", " +
      std::to_string(led_brightness[5]));
  err = TL6WL_setLED_HeadBrightness(instr, led_brightness[0], led_brightness[1],
                                    led_brightness[2], led_brightness[3],
                                    led_brightness[4], led_brightness[5]);
  if (VI_SUCCESS != err) {
    throw std::runtime_error(
        "PulseChainBatch::setUpThisBatch(): Error setting LED head "
        "brightness.");
  }
  logger_ptr->trace("PulseChainBatch setUpThisBatch() done.");
}

char* PulseChainBatch::toChars(const std::string& prefix,
                               const std::string& step_level_prefix) {
  return ProtocolBatch::batchToChars(batch_type, prefix, step_level_prefix);
}
