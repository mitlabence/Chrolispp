#ifndef PROTOCOL_PLANNER_HPP
#define PROTOCOL_PLANNER_HPP
#include <Windows.h>

#include <vector>

#include "Logger.hpp"
#include "ProtocolBatch.hpp"
#include "ProtocolStep.hpp"
#include "TL6WL.h"

enum ValidationResult {
  VALID_STEP = 0,
  INVALID_LED_INDEX = 1,
  INVALID_PULSE_COUNT = 2,
  INVALID_BRIGHTNESS = 3
};

class ProtocolPlanner {
 public:
  ProtocolPlanner(ViSession instr, std::vector<ProtocolStep> protocolSteps,
                  Logger* logger_ptr);
  const std::vector<ProtocolStep>& getSteps() const { return steps; }
  void setUpDevice();
  void executeProtocol();
  char* toChars(const std::string& prefix,
                const std::string& batch_level_prefix,
                const std::string& step_level_prefix);

 private:
  ViSession instr;
  bool batches_loaded = false;
  bool device_set_up = false;
  int i_next_batch_to_execute = 0;
  std::vector<ProtocolStep> steps;
  size_t n_steps;
  std::unique_ptr<ProtocolBatch> getNextBatch(int& step_cursor);
  ValidationResult validateStep(ProtocolStep& step);
  Logger* logger_ptr;
  void shutDownDevice();

  std::vector<std::unique_ptr<ProtocolBatch>> batches;
  void mergeSteps(std::vector<ProtocolStep>& protocolSteps);
  std::vector<std::unique_ptr<ProtocolBatch>> translateToBatches();
};
#endif  // PROTOCOL_PLANNER_HPP