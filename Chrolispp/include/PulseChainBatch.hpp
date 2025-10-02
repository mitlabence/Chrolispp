#ifndef PULSE_CHAIN_BATCH_HPP
#define PULSE_CHAIN_BATCH_HPP
#include <vector>

#include "ProtocolBatch.hpp"

class PulseChainBatch : public ProtocolBatch {
 public:
  PulseChainBatch(ViSession instr, const std::vector<ProtocolStep>& steps);

  std::chrono::milliseconds getBusyDurationMs() const override;
  std::chrono::milliseconds getTotalDurationMs() const override;
  std::chrono::microseconds execute() override;
  void setUpNextBatch(ProtocolBatch& next_batch) override;
  void setUpThisBatch() override;

 private:
  int led_mask = 0;
};

#endif  // PULSE_CHAIN_BATCH_HPP
