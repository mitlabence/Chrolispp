#ifndef INITIAL_BREAK_BATCH_HPP
#define INITIAL_BREAK_BATCH_HPP
#include <vector>

#include "ProtocolBatch.hpp"

class InitialBreakBatch : public ProtocolBatch {
 public:
  InitialBreakBatch(ViSession instr, const std::vector<ProtocolStep>& steps,
                    Logger* logger_ptr);

  std::chrono::milliseconds getBusyDurationMs() const override;
  std::chrono::milliseconds getTotalDurationMs() const override;
  std::chrono::microseconds execute() override;
  void setUpNextBatch(ProtocolBatch& next_batch) override;
  void setUpThisBatch() override;
  char* toChars(const std::string& prefix,
                const std::string& step_level_prefix) override;
};

#endif  // INITIAL_BREAK_BATCH_HPP
