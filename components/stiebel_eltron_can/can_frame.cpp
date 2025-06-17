#include "can_frame.h"

namespace esphome {
namespace stiebel_eltron_can {

CANId CanFrame::get_receiver_id() const {
    // 0x.79 isn't an ID (passive request)
    if (len < 2 || data[1] == 0x79)
        return 0xFFFF;

    return ((data[0] & 0xf0) << 3) + (data[1] & 0x7f);
}

uint16_t CanFrame::get_elster_idx() const {
    if (len > 7 || len < 3) {
        return 0xFFFF;
    }

    if (data[2] == 0xfa) {
        if (len < 5) {
            return 0xFFFF;
        }

        return 256*data[3] + data[4];
    } else {
        return data[2];
    }
}

uint16_t CanFrame::get_value() const {
    if (len < 5 || len > 7) {
        return 0x8000;
    }

    if (data[2] == 0xfa) {
        if (len != 7) {
            return 0x8000;
        }

        return 256*data[5] + data[6];
    }

    return 256*data[3] + data[4];
}

void CanFrame::format(char *buf) const {
    sprintf(buf, "  %8x [%d] ", id & 0x1fffffff, len);

    for (int i = 0; i < 8; i++) {
        if (i < len) {
            sprintf(buf + strlen(buf), "%2.2x ", data[i]);
        } else {
            sprintf(buf + strlen(buf), "   ");
        }
    }
    strcat(buf, " ");

    for (int i = 0; i < 8; i++) {
        if (i < len) {
            sprintf(buf + strlen(buf), "%c",
                    ' ' <= data[i] && data[i] < 127 ? data[i] : '.');
        } else {
            sprintf(buf + strlen(buf), " ");
        }
    }

    sprintf(buf + strlen(buf), "  0x%04x", get_elster_idx());
    if ((data[0] & 0x0f) == 0) {
        sprintf(buf + strlen(buf), " <- %5d", get_value());
    } else if ((data[0] & 0x0f) == 1) {
        sprintf(buf + strlen(buf), " ??      ");
    } else if ((data[0] & 0x0f) == 2) {
        sprintf(buf + strlen(buf), " == %5d", get_value());
    }

    sprintf(buf + strlen(buf), "  (%3x -> %3x)", id & 0x1fffffff, get_receiver_id());
    strcat(buf, "\n");
}

void CanFrame::send(canbus::Canbus *canbus) const {
    std::vector<uint8_t> data_vec(data, data + len);
    canbus->send_data(id, false, data_vec);
}

CanFrame create_read_frame(
    const CanMember &id,
    const CanMember &target,
    const uint16_t elster_index
) {
    CanFrame frame;
    frame.id = id.canId;
    frame.len = 7;

    const uint16_t read_id = target.get_read_id();

    frame.data[0] = read_id >> 8;
    frame.data[1] = read_id & 0xFF;
    frame.data[2] = 0xFA;
    frame.data[3] = elster_index >> 8;
    frame.data[4] = elster_index & 0xFF;
    frame.data[5] = 0x00;
    frame.data[6] = 0x00;

    return frame;
}

CanFrame create_write_frame(
    const CanMember &id,
    const CanMember &target,
    const uint16_t elster_index,
    const uint16_t value
) {
    CanFrame frame;
    frame.id = id.canId;
    frame.len = 7;

    const uint16_t write_id = target.get_write_id();

    frame.data[0] = write_id >> 8;
    frame.data[1] = write_id & 0xFF;
    frame.data[2] = 0xFA;
    frame.data[3] = elster_index >> 8;
    frame.data[4] = elster_index & 0xFF;
    frame.data[5] = value >> 8;
    frame.data[6] = value & 0xFF;

    return frame;
}

}
}