#ifndef INITIAL_BREAK_BATCH_HPP
#define INITIAL_BREAK_BATCH_HPP
#include <vector>

#include "ProtocolBatch.hpp"

class InitialBreakBatch : public ProtocolBatch {
 public:
  InitialBreakBatch(ViSession instr, const std::vector<ProtocolStep>& steps);

  std::chrono::milliseconds getBusyDurationMs() const override;
  std::chrono::milliseconds getTotalDurationMs() const override;
  std::chrono::microseconds execute() override;
  void setUpNextBatch(
      ProtocolBatch& next_batch) override;
  void setUpThisBatch() override;
};

#endif  // INITIAL_BREAK_BATCH_HPP
