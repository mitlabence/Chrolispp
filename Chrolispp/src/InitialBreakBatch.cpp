#include "InitialBreakBatch.hpp"

#include <stdexcept>

#include "Logger.hpp"
#include "Timing.hpp"
#include "constants.hpp"

InitialBreakBatch::InitialBreakBatch(unsigned short batch_id, ViSession instr,
                                     const std::vector<ProtocolStep>& steps,
                                     Logger* logger_ptr)
    : ProtocolBatch(batch_id, instr, steps, logger_ptr) {
  if (steps.empty()) {
    throw std::invalid_argument("No protocol steps provided.");
  }
  if (!steps[0].isBreak() || steps.size() != 1) {
    throw std::invalid_argument(
        "InitialBreakBatch must contain a single break step.");
  }
  busy_duration_us =
      std::chrono::microseconds(0);  // LED is not busy during a break
  total_duration_us =
      std::chrono::microseconds(steps[0].time_between_pulses_us);
  batch_type = "InitialBreakBatch";
}

std::chrono::microseconds InitialBreakBatch::getBusyDurationUs() const {
  return busy_duration_us;
}

std::chrono::microseconds InitialBreakBatch::getTotalDurationUs() const {
  return total_duration_us;
}

std::chrono::microseconds InitialBreakBatch::execute() {
  // TODO: this execute feels like a waste of computing time...
  logger_ptr->trace("InitialBreakBatch execute() (no action)");
  if (execute_attempted) {
    throw std::logic_error(
        "InitialBreakBatch: attempting to execute already executed batch.");
  }
  auto start = std::chrono::high_resolution_clock::now();
  execute_attempted = true;
  logger_ptr->trace("InitialBreakBatch execute() done.");
  // sleep for busy duration, which is 0 ms
  auto end = std::chrono::high_resolution_clock::now();
  // Calculate duration
  auto actual_duration_us =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  busy_duration_us = actual_duration_us;  // update actual busy duration
  return actual_duration_us;
}

void InitialBreakBatch::setUpNextBatch(ProtocolBatch& next_batch) {
  auto start = std::chrono::high_resolution_clock::now();
  logger_ptr->trace("InitialBreakBatch setUpNextBatch()");
  // TODO: avoid repeating this code in other implementations of ProtocolBatch
  if (!execute_attempted) {
    throw std::logic_error(
        "Cannot set up next batch before executing this batch.");
  }
  next_batch.setUpThisBatch();
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::microseconds duration =
      std::chrono::duration_cast<std::chrono::microseconds>(
          end - start);  // Truncate to milliseconds
  // Wait for rest of the (busy - total duration) time if any left after
  // <duration> time passed Convert duration to nearest milliseconds int
  if (duration < total_duration_us - busy_duration_us) {
    std::chrono::milliseconds total_duration_ms =         std::chrono::duration_cast<std::chrono::milliseconds>(
        total_duration_us - busy_duration_us - duration +
		std::chrono::microseconds(999));
    Timing::precise_sleep_for(total_duration_ms);
  }
  logger_ptr->trace("InitialBreakBatch setUpNextBatch() done.");
}

void InitialBreakBatch::setUpThisBatch() {
  logger_ptr->trace("InitialBreakBatch setUpThisBatch() (no action) done.");
  return;
}

#include <cstring>  // Ensure this header is included for string manipulation functions

char* InitialBreakBatch::toChars(const std::string& prefix,
                                 const std::string& step_level_prefix) {
  return ProtocolBatch::batchToChars("InitialBreakBatch", prefix,
                                     step_level_prefix);
}