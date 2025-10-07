#ifndef PULSE_CHAIN_BATCH_HPP
#define PULSE_CHAIN_BATCH_HPP
#include <vector>

#include "Logger.hpp"
#include "ProtocolBatch.hpp"

class PulseChainBatch : public ProtocolBatch {
 public:
  PulseChainBatch(ViSession instr, const std::vector<ProtocolStep>& steps,
                  Logger* logger_ptr);

  std::chrono::milliseconds getBusyDurationMs() const override;
  std::chrono::milliseconds getTotalDurationMs() const override;
  std::chrono::microseconds execute() override;
  void setUpNextBatch(ProtocolBatch& next_batch) override;
  void setUpThisBatch() override;
  char* toChars(const std::string& prefix,
                const std::string& step_level_prefix) override;

 private:
  int led_mask = 0;
};

#endif  // PULSE_CHAIN_BATCH_HPP
