#ifndef SINGLES_BATCH_HPP
#define SINGLES_BATCH_HPP
#include <vector>

#include "ProtocolBatch.hpp"

class SinglePulsesBatch : public ProtocolBatch {
 public:
  SinglePulsesBatch(ViSession instr, const std::vector<ProtocolStep>& steps);

  std::chrono::milliseconds getBusyDurationMs() const override;
  std::chrono::milliseconds getTotalDurationMs() const override;
  std::chrono::microseconds execute() override;
  void setUpNextBatch(ProtocolBatch& next_batch) override;
  void setUpThisBatch() override;

 private:
  int led_mask = 0;  // bit mask of all used LEDs in protocol
};

#endif  // SINGLES_BATCH_HPP
