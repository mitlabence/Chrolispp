#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

namespace Constants {
constexpr int BATCH_HEADER_CHARS_BUFFERSIZE =
    48;  // Buffer size for ProtocolBatch header
constexpr int STEP_CHARS_BUFFERSIZE =
    128;  // Buffer size for ProtocolStep printing function
constexpr int PROTOCOL_PLANNER_HEADER_CHARS_BUFFERSIZE = 30;
}  // namespace Constants

#endif  // CONSTANTS_HPP
