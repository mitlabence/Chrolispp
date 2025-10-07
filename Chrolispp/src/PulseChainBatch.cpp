#include "PulseChainBatch.hpp"

#include "Timing.hpp"
#include "constants.hpp"
PulseChainBatch::PulseChainBatch(ViSession instr,
                                 const std::vector<ProtocolStep>& steps,
                                 Logger* logger_ptr)
    : ProtocolBatch(instr, steps, logger_ptr) {}

std::chrono::milliseconds PulseChainBatch::getBusyDurationMs() const {
  return busy_duration_ms;
}

std::chrono::milliseconds PulseChainBatch::getTotalDurationMs() const {
  return total_duration_ms;
}

std::chrono::microseconds PulseChainBatch::execute() {
  logger_ptr->trace("PulseChainBatch execute()");
  if (executed) {
    throw std::logic_error(
        "PulseChainBatch: attempting to execute already executed batch.");
  }
  auto start = std::chrono::high_resolution_clock::now();
  ViStatus err;
  // Start the timer
  logger_ptr->protocol("Executing PulseChainBatch with steps:");
  logger_ptr->multiLineProtocol(toChars("\t", "\t\t"));
  err = TL6WL_TU_StartStopGeneratorOutput_TU(instr, true);
  if (VI_SUCCESS != err) {
    throw std::runtime_error(
        "PulseChainBatch::execute(): Error starting signal generator.");
  }
  Timing::precise_sleep_for(busy_duration_ms);
  executed = true;
  logger_ptr->trace("PulseChainBatch execute() done.");
  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  return duration;
}

void PulseChainBatch::setUpNextBatch(ProtocolBatch& next_batch) {
  auto start = std::chrono::high_resolution_clock::now();
  logger_ptr->trace("PulseChainBatch setUpNextBatch()");
  // TODO: avoid repeating this code in other implementations of ProtocolBatch
  if (!executed) {
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
  if (duration < total_duration_ms - busy_duration_ms) {
    Timing::precise_sleep_for(total_duration_ms - busy_duration_ms - duration);
  }
  logger_ptr->trace("PulseChainBatch setUpNextBatch() done.");
}

void PulseChainBatch::setUpThisBatch() {
  logger_ptr->trace("PulseChainBatch setUpThisBatch()");
  // Loop over steps, set up periodic signal and keep track of delay times
  ViStatus err;
  // Turn off LEDs
  // TODO: is this necessary? To avoid getting stuck on high state during
  // setup... Or just send brightness 0?
  err = TL6WL_setLED_HeadPowerStates(instr, VI_FALSE, VI_FALSE, VI_FALSE,
                                     VI_FALSE, VI_FALSE, VI_FALSE);
  // Stop timer
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
  for (const auto& step :
       protocol_steps) {  // TODO: set each brightness value first, then set the
                          // self running signals? I did it like that in demo...
                          // Then need to loop twice?
    int led_index = step.led_index;
    int brightness = step.brightness;
    led_brightness[led_index] = brightness;
    led_mask |= (1 << led_index);
    err = TL6WL_TU_AddGeneratedSelfRunningSignal(
        instr, step.led_index + 1, VI_FALSE, duration_so_far_us,
        step.pulse_width_ms * 1000, step.time_between_pulses_ms * 1000,
        step.n_pulses);
    if (VI_SUCCESS != err) {
      throw std::runtime_error(
          "PulseChainBatch::setUpThisBatch(): Error adding signal to signal "
          "generator.");
    }
    // Add breakout box signal as well
    err = TL6WL_TU_AddGeneratedSelfRunningSignal(
        instr, step.led_index + 1 + 6, VI_FALSE, duration_so_far_us,
        step.pulse_width_ms * 1000, step.time_between_pulses_ms * 1000,
        step.n_pulses);
    if (VI_SUCCESS != err) {
      throw std::runtime_error(
          "PulseChainBatch::setUpThisBatch(): Error adding signal to signal "
          "generator.");
    }
    duration_so_far_us += step.getTotalDurationMs() * 1000;
  }
  err = TL6WL_setLED_HeadBrightness(instr, led_brightness[0], led_brightness[1],
                                    led_brightness[2], led_brightness[3],
                                    led_brightness[4], led_brightness[5]);
  if (VI_SUCCESS != err) {
    throw std::runtime_error(
        "SinglePulsesBatch::setUpThisBatch(): Error setting LED head "
        "brightness.");
  }
  logger_ptr->trace("PulseChainBatch setUpThisBatch() done.");
}

char* PulseChainBatch::toChars(const std::string& prefix,
                               const std::string& step_level_prefix) {
  return ProtocolBatch::batchToChars("PulseChainBatch", prefix, step_level_prefix);
}
