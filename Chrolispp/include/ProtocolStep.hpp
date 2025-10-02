#ifndef PROTOCOL_STEP_HPP
#define PROTOCOL_STEP_HPP

#include "TL6WL.h"  // class is specific to this equipment (ThorLabs 6 LED machine)

class ProtocolStep {
 public:
  ProtocolStep(ViUInt16 led_index, ViUInt32 pulse_width_ms,
               ViUInt32 time_between_pulses_ms,
                ViUInt32 n_pulses, ViUInt16 brightness);

  ViUInt16 led_index;  // LED index (0-5 for TL6WL)
  ViUInt32 pulse_width_ms;
  ViUInt32 time_between_pulses_ms;
  ViUInt32 n_pulses;
  ViUInt16 brightness;
  bool isGaplessSinglePulse() const;
  bool isBreak() const;
  ViUInt32 getBreakDurationMs() const;
  ViUInt32 getTotalDurationMs() const;
  void setBreakDuration(int break_duration_ms);
  void printStep();
  char* stepToChars();
};

#endif  // PROTOCOL_STEP_HPP
