#pragma once

#include <cstdint>

namespace esphome {
namespace stiebel_eltron_can {

enum Type : uint8_t {
    et_default = 0,  //   + x
    et_dec_val,      // +/- xx.x
    et_cent_val,     // +/- x.xx
    et_mil_val,      // +/- x.xxx
    et_byte,
    et_bool,         // 0x0000 und 0x0001
    et_little_bool,  // 0x0000 und 0x0100
    et_double_val,
    et_triple_val,      // Ordering: 10^6, 10^3, 10^0
    et_inv_double_val,
    et_inv_triple_val,  // Ordering: 10^0, 10^3, 10^6
    et_little_endian,
    et_betriebsart,
    et_zeit,
    et_datum,
    et_time_domain,
    et_dev_nr,
    et_err_nr,
    et_dev_id
};

}
}
