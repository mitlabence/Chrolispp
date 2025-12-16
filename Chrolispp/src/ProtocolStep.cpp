#include "ProtocolStep.hpp"

#include <cstring>  // Include for strcpy
#include <iostream>

#include "constants.hpp"
#include "DurationAndUnit.hpp"

/// <summary>
/// 
/// </summary>
/// <param name="step_id">The identifier of the step</param>
/// <param name="led_index">The LED index</param>
/// <param name="pulse_width">The pulse width (in us if is_us_mode, otherwise in ms)</param>
/// <param name="time_between_pulses">Time between pulses (in us if is_us_mode, otherwise in ms)</param>
/// <param name="n_pulses">The number of pulses</param>
/// <param name="brightness">The brightness (0-1000; 0=0.0%, 123 = 12.3%, 1000=100.0%)</param>
/// <param name="is_us_mode">Whether the step time characteristics are defined in units of us. (If false: ms.)</param>
ProtocolStep::ProtocolStep(unsigned short step_id, ViUInt16 led_index,
                           ViUInt32 pulse_width,
                           ViUInt32 time_between_pulses, ViUInt32 n_pulses,
                           ViUInt16 brightness, bool is_us_mode=false)
    : step_id(step_id) {
  if (brightness ==
      0) {  // Unify break notation: either pulse_width 0, or brightness = 0; in
            // latter case, pulse duration also adds to break. To avoid handling
            // this possibility in other places, unify from start. Thus break
            // always has 0 pulse_width_ms, and have 0 brightness. Also, ignore
            // n_pulses, make the break only once. To make it more obvious, set
            // n_pulses to 1 if break is detected.
    time_between_pulses += pulse_width;
    pulse_width = 0;
    n_pulses = 1;
    led_index = 0;  // TODO: if break detected, make led_index invalid?
  } else if (pulse_width == 0) {
    brightness = 0;
    n_pulses = 1;
    led_index = 0;
  }
  // If a gapless single pulse is wanted (i.e. with no trailing break), n_pulses
  // is taken as the multiplier (i.e. the total pulse duration is n_pulses *
  // pulse_width_ms). I.e. 2 pulses of 100 ms with no break is the same as 1
  // pulse of 200 ms.
  if (time_between_pulses == 0) {
    pulse_width *= n_pulses;
    n_pulses = 1;
  }
  this->led_index = led_index;

  this->brightness = brightness;
  this->n_pulses = n_pulses;
  this->is_us_mode = is_us_mode;
  if (is_us_mode) {
      // in us mode, pulse width and time between pulses should be multiples of 5us
      if (pulse_width % 5 != 0 || time_between_pulses % 5 != 0) {
          throw std::invalid_argument("In us mode, pulse width and time between pulses must be multiples of 5 us.");
	  }
      this->pulse_width_us = pulse_width;
      this->time_between_pulses_us = time_between_pulses;
  }
  else { // ms mode, convert to us
      this->pulse_width_us = pulse_width * 1000;
	  this->time_between_pulses_us = time_between_pulses * 1000;
  }
}

/*
 Check if the step is a single pulse with no trailing break (i.e. time between
 pulses = 0 ms). From the constructor, it is guaranteed that in this case
 n_pulses == 1.
*/
bool ProtocolStep::isGaplessSinglePulse() const {
  return (time_between_pulses_us == 0);
}
/*
    After the unification of the break definition in the constructor, use this
    function to check whether the step is a break.*/
bool ProtocolStep::isBreak() const { return (brightness == 0); }
ViUInt32 ProtocolStep::getBreakDurationUs() const {
  if (isBreak()) {
    if(is_us_mode) {  // time_between_pulses already us
        return time_between_pulses_us;
    }
    else { // convert ms to us
        return time_between_pulses_us * 1000;
	}
  } else {
    throw std::logic_error(
        "getBreakDuration() called on non-break ProtocolStep.");
  }
}
void ProtocolStep::setBreakDuration(int break_duration_us) {
  if (isBreak()) {
    time_between_pulses_us = break_duration_us;
  } else {
    throw std::logic_error(
        "setBreakDuration() called on non-break ProtocolStep.");
  }
}

void ProtocolStep::printStep() {
  // FIXME: use unified definition of break
  // If pulse width is 0, it means a break
	DurationAndUnit pulse_width_dau = findDurationAndUnit(pulse_width_us);
	DurationAndUnit time_between_pulses_dau = findDurationAndUnit(time_between_pulses_us);
  if (pulse_width_us == 0) {
    std::cout << "Break (id " << step_id
              << "), duration: " << time_between_pulses_dau.duration << " " << time_between_pulses_dau.unit << std::endl;
  } else if (brightness == 0) {
	  DurationAndUnit total_break_dau = findDurationAndUnit(pulse_width_us + time_between_pulses_us);
    std::cout << "Break (id " << step_id
		<< "), duration: " << total_break_dau.duration
              << " " << total_break_dau.unit << std::endl;
  } else {
    std::cout << "Step id " << step_id << ", "
              << "LED index: " << led_index << ", "
              << "Pulse width: " << pulse_width_dau.duration << " " << pulse_width_dau.unit << ", "
              << "Time between pulses: " << time_between_pulses_dau.duration << " " << time_between_pulses_dau.unit << ", "
              << "Number of pulses: " << n_pulses << ", "
              << "Brightness: " << brightness << std::endl;
  }
}

char* ProtocolStep::toChars(const std::string& prefix) {
  // Calculate the required buffer size
  const int bufferSize = Constants::STEP_CHARS_BUFFERSIZE;
  char* stepChars = new char[bufferSize];

  // Format the string into the buffer
  if (pulse_width_us == 0) {
	  DurationAndUnit time_between_dau = findDurationAndUnit(time_between_pulses_us);
    std::snprintf(stepChars, bufferSize,
                  "%sStep (id %hu) Break, duration: %d %s", prefix.c_str(),
                  step_id, time_between_dau.duration, time_between_dau.unit.c_str());
  } else if (brightness == 0) {
	  DurationAndUnit total_break_dau = findDurationAndUnit(pulse_width_us + time_between_pulses_us);
    std::snprintf(stepChars, bufferSize, "%sBreak (id %hu), duration: %d %s",
                  prefix.c_str(), step_id,
                  total_break_dau.duration, total_break_dau.unit.c_str());
  } else {
	  DurationAndUnit pulse_width_dau = findDurationAndUnit(pulse_width_us);
	  DurationAndUnit time_between_dau = findDurationAndUnit(time_between_pulses_us);
    std::snprintf(
        stepChars, bufferSize,
        "%sStep (id %hu): LED index: %d, Pulse width: %d %s, Time between "
        "pulses: %d %s, "
        "Number of pulses: %d, Brightness: %d",
        prefix.c_str(), step_id, led_index, pulse_width_dau.duration, pulse_width_dau.unit.c_str(),
        time_between_dau.duration, time_between_dau.unit.c_str(), n_pulses, brightness);
  }
  return stepChars;
}

ViUInt32 ProtocolStep::getTotalDurationUs() const {
    if (isBreak()) {
        return time_between_pulses_us;
    }
    else {
        return n_pulses * (pulse_width_us + time_between_pulses_us);
    }
}