#ifndef CAPSLOCK_IDS_HPP
#define CAPSLOCK_IDS_HPP

#include <cstdint>

constexpr const char* CAPSLOCK_FILE_PATH = 
    "/sys/class/leds/input4::capslock/brightness";

// Example 01: Control (Request/Response)
namespace control {
    constexpr uint16_t SERVICE_ID  = 0x1111;
    constexpr uint16_t INSTANCE_ID = 0x0001;
    constexpr uint16_t METHOD_SET  = 0x0001;
}

// Example 02: Monitor (Event/Notify)
namespace monitor {
    constexpr uint16_t SERVICE_ID    = 0x2222;
    constexpr uint16_t INSTANCE_ID   = 0x0001;
    constexpr uint16_t EVENT_ID      = 0x8001;
    constexpr uint16_t EVENTGROUP_ID = 0x0001;
}

#endif