#include <vector>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "esphome/components/canbus/canbus.h"
#include "can_member.h"

namespace esphome {
namespace stiebel_eltron_can {

struct CanFrame {
    CANId id;
    uint8_t len;
    uint8_t data[8];

    CanFrame() : id(0), len(0) {
        for (int i = 0; i < 8; ++i) {
            data[i] = 0;
        }
    }

    CanFrame(CANId id, const std::vector<uint8_t> &data, uint8_t len) : id(id), len(len < 8 ? len : 8) {
        for (int i = 0; i < this->len; ++i) {
            this->data[i] = data[i];
        }
    }

    CANId get_receiver_id() const;
    uint16_t get_elster_idx() const;
    uint16_t get_value() const;

    void format(char *buf) const;
    void send(canbus::Canbus *canbus) const;
};

CanFrame create_read_frame(
    const CanMember &id,
    const CanMember &target,
    const uint16_t elster_index
);

CanFrame create_write_frame(
    const CanMember &id,
    const CanMember &target,
    const uint16_t elster_index,
    const uint16_t value
);

}
}