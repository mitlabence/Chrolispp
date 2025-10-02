#ifndef TIMING_HPP
#define TIMING_HPP
#include <chrono>

namespace Timing {
void precise_sleep_for(std::chrono::milliseconds ms);
}

#endif  // TIMING_HPP
