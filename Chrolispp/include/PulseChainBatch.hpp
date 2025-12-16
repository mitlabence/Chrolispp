#ifndef PULSE_CHAIN_BATCH_HPP
#define PULSE_CHAIN_BATCH_HPP
#include <vector>

#include "Logger.hpp"
#include "ProtocolBatch.hpp"

/*
PulseChainBatch: Executes a series of steps with no configuration necessary
in-between. The steps must
1. Each have a different LED index (or be a break)
2. Each may have 1 or more pulses (n_pulses >= 1) if not a break
3. There must be a time between pulses that is > 0 (i.e. no "gapless pulses";
for this, GaplessPulseBatch must be used)
4. A break may only occur at the end of the batch (i.e. the last step may be a
break)
*/

class PulseChainBatch : public ProtocolBatch {
 public:
  PulseChainBatch(unsigned short batch_id, ViSession instr,
                  const std::vector<ProtocolStep>& steps, Logger* logger_ptr);

  std::chrono::microseconds getBusyDurationUs() const override;
  std::chrono::microseconds getTotalDurationUs() const override;
  std::chrono::microseconds execute() override;
  void setUpNextBatch(ProtocolBatch& next_batch) override;
  void setUpThisBatch() override;
  char* toChars(const std::string& prefix,
                const std::string& step_level_prefix) override;

 private:
  int led_mask = 0;
  bool has_trailing_break = false;  // Whether there
};

#endif  // PULSE_CHAIN_BATCH_HPP
