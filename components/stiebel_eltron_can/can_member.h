#pragma once

#include <string>
#include <cstdint>

namespace esphome {
namespace stiebel_eltron_can {

using CANId = uint16_t;
struct CanMember {
    CANId canId;
    std::string name;
    uint16_t get_write_id() const { return (((canId & 0x7c0) << 5U) + (canId & 0x3f)); }
    uint16_t get_read_id() const { return get_write_id() | 0x100; }
    uint16_t get_response_id() const { return get_write_id() | 0x200; }
};

namespace can_members {
    static const CanMember ESPClient{0x6a2, "ESPClient"};
    static const CanMember Manager{0x514, "Manager"};
    static const CanMember Kessel{0x180, "Kessel"};
    static const CanMember HK1{0x601, "HK1"};
    static const CanMember HK2{0x602, "HK2"};
    static const CanMember FET{0x402, "FET"};
    static const CanMember MFG{0x700, "MFG"};
    static const CanMember WPM{0x480, "WPM"};
    static const CanMember WWTEMP{0x201, "WWTEMP"};
}

}
}