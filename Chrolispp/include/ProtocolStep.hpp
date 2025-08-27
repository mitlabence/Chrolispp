#ifndef PROTOCOL_STEP_HPP
#define PROTOCOL_STEP_HPP

#include "TL6WL.h"  // class is specific to this equipment (ThorLabs 6 LED machine)

class ProtocolStep {
 public:
  ProtocolStep(ViInt16 led_index, ViInt32 pulse_width_ms,
               ViInt32 time_between_pulses_ms,
                ViInt32 n_pulses, ViUInt16 brightness);

  ViInt16 led_index;
  ViInt32 pulse_width_ms;
  ViInt32 time_between_pulses_ms;
  ViInt32 n_pulses;
  ViUInt16 brightness;

  void printStep();
  char* stepToChars();
};

#endif  // PROTOCOL_STEP_HPP
