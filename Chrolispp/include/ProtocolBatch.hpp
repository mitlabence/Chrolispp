/*
* An interface for batch classes with custom setup, execution ... logic.
A batch is a group of protocol steps that can be programmed at once. It may not
start with a break (except possibly the first batch in the sequence, as the
setup of the first step is not time-critical), and may or may not end with a
break (this is to help setting up the next batch in the LED-off time of the
previous batch to reduce timing delays). These are the fundamentally different
types:
- Sequence of pulse trains/chains: each LED can be used only once in the batch,
  i.e. if a step uses an LED that is already used in the batch, this step
  should be in the next batch (this is the restriction of the LED machine).
  Breaks (0 brightness) can be anywhere in the batch except in the very end.
- Sequence of single pulses, i.e. n_pulses = 1; they can be with or without a
    trailing break (the latter is called gapless). These can be controlled in a
    different manner (directly manipulating brightness without reprogramming the
LED machine and without using its internal clock). The timing is handled inside
this software.
*/

#ifndef PROTOCOL_BATCH_HPP
#define PROTOCOL_BATCH_HPP
#include <chrono>
#include <vector>

#include "ProtocolStep.hpp"
/*
 Usage:
 - Create a derived class from ProtocolBatch, implementing the pure virtual
  functions.
  - Create an instance of the derived class, passing a vector of ProtocolStep
  instances to the constructor.
  - If only one batch:
    - Call setUpThisBatch() to set up the batch (e.g. program the LED
    machine).
    - Call execute() to execute the batch. This returns the actual time taken
    in microseconds.
  - If multiple batches: use a std::vector of pointers to the batch instances.
      - For the first batch:
          - Call current_batch.setUpThisBatch() to set up the batch (e.g.
            program the LED machine). This step is not time-critical, as the
            sequence is not yet running.
          - Call execute() to execute the first batch. This starts the first
 batch (and the time critical part). execute() returns after approximately
 busy_duration_ms time, but the function returns a more precise (?)
 approximation of the actual duration.
          - Call setUpNextBatch(next_batch). This automatically waits for the
            rest of the duration remaining from the total_duration_ms (possibly
            taking up more time than that)
    - For each subsequent batch:
        - Call execute() on the current batch.
        - Call current_batch.setUpNextBatch(next_batch) (if available).
*/
class ProtocolBatch {
 public:
  ProtocolBatch(ViSession instr, const std::vector<ProtocolStep>& steps)
      : instr(instr),
        protocol_steps(steps),
        busy_duration_ms(0),
        total_duration_ms(0) {};
  virtual ~ProtocolBatch() = default;

  virtual std::chrono::milliseconds getBusyDurationMs() const = 0;
  virtual std::chrono::milliseconds getTotalDurationMs() const = 0;
  virtual std::chrono::microseconds execute() = 0;
  /*
  set_up_next_batch() should be called after execute(), in the time between
  busy_duration_ms elapsed and the total total_duration_ms. It should set up
  the next batch (e.g. program the LED machine) so that it is ready to be
  started immediately after total_duration_ms elapsed. set_up_next_batch()
  throws an error if called before execute().
  */
  virtual void setUpNextBatch(
      ProtocolBatch& next_batch) = 0;  // TODO: this function is implemented the
                                       // same way for each implementation! Need
                                       // to avoid repeating code...
  virtual void setUpThisBatch() = 0;

 protected:
  ViSession instr;
  std::vector<ProtocolStep> protocol_steps;
  std::chrono::milliseconds busy_duration_ms;
  std::chrono::milliseconds total_duration_ms;
  bool executed = false;
};

#endif  // PROTOCOL_BATCH_HPP
