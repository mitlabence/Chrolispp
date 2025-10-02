#include "InitialBreakBatch.hpp"

#include <stdexcept>

#include "Timing.hpp"
InitialBreakBatch::InitialBreakBatch(ViSession instr,
                                     const std::vector<ProtocolStep>& steps)
    : ProtocolBatch(instr, steps) {
  if (steps.empty()) {
    throw std::invalid_argument("No protocol steps provided.");
  }
  if (!steps[0].isBreak() || steps.size() != 1) {
    throw std::invalid_argument(
        "InitialBreakBatch must contain a single break step.");
  }
  busy_duration_ms =
      std::chrono::milliseconds(0);  // LED is not busy during a break
  total_duration_ms =
      std::chrono::milliseconds(steps[0].time_between_pulses_ms);
}

std::chrono::milliseconds InitialBreakBatch::getBusyDurationMs() const {
  return busy_duration_ms;
}

std::chrono::milliseconds InitialBreakBatch::getTotalDurationMs() const {
  return total_duration_ms;
}

std::chrono::microseconds InitialBreakBatch::execute() {
  auto start = std::chrono::high_resolution_clock::now();
  Timing::precise_sleep_for(busy_duration_ms);
  auto end = std::chrono::high_resolution_clock::now();

  // Calculate duration
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  executed = true;
  return duration;
}

void InitialBreakBatch::setUpNextBatch(ProtocolBatch& next_batch) {
  // TODO: avoid repeating this code in other implementations of ProtocolBatch
  if (!executed) {
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
}

void InitialBreakBatch::setUpThisBatch() { return; }
