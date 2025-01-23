#include "ProtocolStep.hpp"
#include <iostream>
// Constructor definition
ProtocolStep::ProtocolStep(ViInt16 led_index, ViInt32 time_between_pulses_ms,
                           ViInt32 pulse_width_ms, ViInt32 n_pulses,
                           ViInt16 brightness)
    : led_index(led_index),
      time_between_pulses_ms(time_between_pulses_ms),
      pulse_width_ms(pulse_width_ms),
      n_pulses(n_pulses),
      brightness(brightness) {


    
}

void ProtocolStep::printStep(){ 
    std::cout << "LED index: " << led_index << ", "
            << "Time between pulses: " << time_between_pulses_ms << " ms, "
            << "Pulse width: " << pulse_width_ms << " ms, "
            << "Number of pulses: " << n_pulses << ", "
            << "Brightness: " << brightness <<
        std::endl;
};
