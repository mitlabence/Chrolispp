#ifndef PROTOCOL_STEP_HPP
#define PROTOCOL_STEP_HPP

#include <string>
#include "TL6WL.h"  // class is specific to this equipment (ThorLabs 6 LED machine)
class ProtocolStep {
 public:
  ProtocolStep(unsigned short step_id, ViUInt16 led_index,
               ViUInt32 pulse_width, ViUInt32 time_between_pulses,
               ViUInt32 n_pulses, ViUInt16 brightness, bool is_us_mode);

  unsigned short step_id;  // unique step ID. Index makes most sense.
  /// <summary>
  /// The LED index (0-5 for TL6WL).
  /// </summary>
  ViUInt16 led_index;
  /// <summary>
  /// The pulse width in us.
  /// </summary>
  ViUInt32 pulse_width_us;
  /// <summary>
  ///  The time between two pulses in us.
  /// </summary>
  ViUInt32 time_between_pulses_us;
  /// <summary>
  /// The number of pulses.
  /// </summary>
  ViUInt32 n_pulses;
  /// <summary>
  /// The brightness setting of the LED machine (0-1000). 0 corresponds to 0.0%, 
  /// e.g. 125 to 12.5%, 1000 to 100.0%.
  /// </summary>
  ViUInt16 brightness;
  /// <summary>
  /// Whether the timing parameters were provided in microseconds (true) or milliseconds (false). If is_us_mode, 
  /// conversion of pulse_width_us and time_between_pulses_us to ms is not safe; furthermore, if is_us_mode,
  /// pulse_width and time_between_pulses must be multiples of 5.
  /// </summary>
  bool is_us_mode;
  bool isGaplessSinglePulse() const;
  /// <summary>
  /// Whether this step is a break (i.e. no pulse, only waiting).
  /// </summary>
  /// <returns>True if step is a break, false otherwise.</returns>
  bool isBreak() const;
  /// <summary>
  /// Get the break duration in microseconds. Only valid if isBreak() is true.
  /// </summary>
  /// <returns>The break duration in microseconds.</returns>
  ViUInt32 getBreakDurationUs() const;
  /// <summary>
  /// Get the total duration of the step in microseconds.
  /// </summary>
  /// <returns>The total duration in microseconds.</returns>
  ViUInt32 getTotalDurationUs() const;
  /// <summary>
  /// Change the break duration of this step to the specified value (in us).
  /// </summary>
  /// <param name="break_duration_us"></param>
  void setBreakDuration(int break_duration_us);
  void printStep();
  char* toChars(const std::string& prefix);
};

#endif  // PROTOCOL_STEP_HPP
