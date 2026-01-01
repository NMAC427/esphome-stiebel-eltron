#define LOG_CAN

#include "stiebel_eltron_can.h"
#include "esphome/core/log.h"

namespace esphome {
namespace stiebel_eltron_can {

static const char *const TAG = "stiebel_eltron_can";

void log_can_message(const CanFrame &frame) {
  #ifdef LOG_CAN
    char buf[512];
    frame.format(buf);
    ESP_LOGI("can.log", "%s", buf);
  #endif
}

void StiebelEltronCanComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Stiebel Eltron CAN component...");

  // Set up CAN message handler
  this->canbus_->add_callback(
    [this](uint32_t can_id, bool extended_id, bool remote_transmission_request, const std::vector<uint8_t> &data) {
      this->process_can_message(can_id, data);
      this->blink_status_led();
    }
  );

  // Periodically check if there is an item in the send queue, and then send the
  // front most. This prevents flooding the CAN bus with requests.
  this->set_interval(50, [this](){
    if (this->send_queue.empty()) return;

    CanFrame frame = this->send_queue.front();
    this->send_queue.pop_front();

    log_can_message(frame);
    frame.send(this->canbus_);
  });
}

void StiebelEltronCanComponent::loop() {}

void StiebelEltronCanComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Stiebel Eltron CAN Component:");
  ESP_LOGCONFIG(TAG, "  # Sensors: %d", this->sensors_.size());
  ESP_LOGCONFIG(TAG, "  # Numbers: %d", this->numbers_.size());
}

void StiebelEltronCanComponent::register_sensor(StiebelEltronCanSensor *sensor) {
  this->sensors_.push_back(sensor);
}

void StiebelEltronCanComponent::register_number(StiebelEltronCanNumber *number) {
  this->numbers_.push_back(number);
}

void StiebelEltronCanComponent::process_can_message(uint32_t can_id, const std::vector<uint8_t> &data) {
  CanFrame frame(can_id, data, data.size());
  log_can_message(frame);

  // Ignore request frames
  if ((data[0] & 0x0F) != 0x02) return;

  // Check if this is an Elster response
  uint16_t elster_index = frame.get_elster_idx();
  if (elster_index != 0xFFFF) {
    for (auto *sensor : this->sensors_) {
      if (sensor->is_failed()) continue;
      sensor->process_can_message(frame);
    }

    for (auto *number : this->numbers_) {
      if (number->is_failed()) continue;
      number->process_can_message(frame);
    }
  }
}

void StiebelEltronCanComponent::request_elster_value(uint16_t elster_index, CanMember target, bool immediately) {
  CanFrame frame = create_read_frame(
      this->sender_,
      target,
      elster_index
  );

  if (immediately) {
    log_can_message(frame);
    frame.send(this->canbus_);
    this->blink_status_led();
  } else {
    send_queue.emplace_back(frame);
  }
}

void StiebelEltronCanComponent::set_elster_value(float value, uint16_t elster_index, Type type, CanMember target) {
  CanFrame frame = create_write_frame(
      this->sender_,
      target,
      elster_index,
      encode_elster_data(type, value)
  );

  log_can_message(frame);
  frame.send(this->canbus_);
  this->blink_status_led();
}

void StiebelEltronCanComponent::blink_status_led() {
#ifdef _SE_USE_STATUS_LED
  auto led = this->status_led_;
  if (!led) return;

  led->turn_on().perform();
  this->defer("status_led_off", [led](){
    led->turn_off().perform();
  });
#endif
}

uint16_t encode_elster_data(Type type, float value) {
  switch (type) {
    case et_dec_val:
      return (uint16_t) ((int16_t) round(value * 10));
    case et_cent_val:
      return (uint16_t) ((int16_t) round(value * 100));
    case et_mil_val:
      return (uint16_t) ((int16_t) round(value * 1000));
    case et_little_endian: {
      uint16_t s = (uint16_t) value;
      return (s >> 8) + 256 * (s & 0xff);
    }
    case et_default:
    default:
      return (uint16_t) value;
  }
}

float decode_elster_data(Type type, uint16_t data) {
  int16_t signed_data = (int16_t) data;

  switch (type) {
    case et_dec_val:
      return (float) signed_data / 10.0;
    case et_cent_val:
      return (float) signed_data / 100.0;
    case et_mil_val:
      return (float) signed_data / 1000.0;
    case et_little_endian:
      return (float) ((data >> 8) + 256 * (data & 0xff));
    case et_default:
    case et_double_val:
    case et_triple_val:
    case et_inv_double_val:
    case et_inv_triple_val:
    default:
      return (float) data;
  }
}

// -- StiebelEltronCanSensor

void StiebelEltronCanSensor::setup() {
  this->update();
}

void StiebelEltronCanSensor::auto_accuracy_decimals() {
  switch (this->get_elster_type()) {
  case et_dec_val:
    this->set_accuracy_decimals(1);
    break;
  case et_cent_val:
    this->set_accuracy_decimals(2);
    break;
  case et_mil_val:
    this->set_accuracy_decimals(3);
    break;

  case et_default:
  case et_little_endian:
  case et_double_val:
  case et_triple_val:
  case et_inv_double_val:
  case et_inv_triple_val:
  default:
    this->set_accuracy_decimals(0);
    break;
  }
}

void StiebelEltronCanSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "Stiebel Eltron CAN Sensor (%s):", this->get_name().c_str());
  ESP_LOGCONFIG(TAG, "  Target: %s", this->target_.name.c_str());
  ESP_LOGCONFIG(TAG, "  Elster Index: 0x%04x", this->get_elster_index());
  ESP_LOGCONFIG(TAG, "  Elster Type: %d", this->get_elster_type());
  LOG_UPDATE_INTERVAL(this);
}

float StiebelEltronCanSensor::combine_value() const {
  switch (this->elster_type_) {
  case et_inv_double_val:
    return (
      std::get<1>(combined_value_) +
      std::get<0>(combined_value_) * 1000.0
    );
  case et_inv_triple_val:
    return (
      std::get<2>(combined_value_) +
      std::get<1>(combined_value_) * 1000.0 +
      std::get<0>(combined_value_) * 1000000.0
    );
  case et_double_val:
  case et_triple_val:
    return (
      std::get<0>(combined_value_) +
      std::get<1>(combined_value_) * 1000.0 +
      std::get<2>(combined_value_) * 1000000.0
    );
  }

  return NAN;
}

void StiebelEltronCanSensor::process_can_message(const CanFrame &frame) {
  if (frame.id != this->target_.canId) return;

  uint16_t elster_index = frame.get_elster_idx();
  switch (this->elster_type_) {
    case et_double_val:
    case et_inv_double_val:
      if (this->elster_index_ != elster_index &&
          this->elster_index_ + 1 != elster_index) return;
      break;
    case et_triple_val:
    case et_inv_triple_val:
    if (this->elster_index_ != elster_index &&
        this->elster_index_ + 1 != elster_index &&
        this->elster_index_ + 2 != elster_index) return;
      break;
    default:
      if (this->elster_index_ != elster_index) return;
      break;
  }

  uint16_t raw_value = frame.get_value();
  if (raw_value == 0x8000) {
    this->publish_state(NAN);
    return;
  }

  float value = decode_elster_data(this->elster_type_, (uint16_t)raw_value);

  switch (this->elster_type_) {
    case et_double_val:
    case et_triple_val:
    case et_inv_double_val:
    case et_inv_triple_val:
      if (elster_index == this->elster_index_) {
        std::get<0>(combined_value_) = value;
      } else if (elster_index == this->elster_index_ + 1) {
        std::get<1>(combined_value_) = value;
      } else if (elster_index == this->elster_index_ + 2) {
        std::get<2>(combined_value_) = value;
      }

      value = this->combine_value();
      if (!isnan(value)) {
        this->publish_state(value);
      }
      break;

    default:
      this->publish_state(value);
      break;
  }
}

void StiebelEltronCanSensor::update() {
  this->parent_->request_elster_value(this->get_elster_index(), this->target_);

  if (this->elster_type_ == et_double_val || this->elster_type_ == et_inv_double_val) {
    this->parent_->request_elster_value(this->get_elster_index() + 1, this->target_);
    combined_value_ = {NAN, NAN, 0};
  } else if (this->elster_type_ == et_triple_val || this->elster_type_ == et_inv_triple_val) {
    this->parent_->request_elster_value(this->get_elster_index() + 1, this->target_);
    this->parent_->request_elster_value(this->get_elster_index() + 2, this->target_);
    combined_value_ = {NAN, NAN, NAN};
  }
}

// -- StiebelEltronCanNumber

void StiebelEltronCanNumber::setup() {
  // Set accuracy
  switch (this->elster_type_) {
  case et_dec_val:
    this->traits.set_step(0.1f);
    break;
  case et_cent_val:
    this->traits.set_step(0.01f);
    break;
  case et_mil_val:
    this->traits.set_step(0.001f);
    break;

  case et_double_val:
  case et_triple_val:
  case et_inv_double_val:
  case et_inv_triple_val:
    this->mark_failed(LOG_STR("Combined entities (double / triple) are not supported."));
    break;

  case et_default:
  case et_little_endian:
  default:
    this->traits.set_step(1.0f);
    break;
  }

  // Initialize with value
  this->update();
}

void StiebelEltronCanNumber::dump_config() {
  ESP_LOGCONFIG(TAG, "Stiebel Eltron CAN Number (%s):", this->get_name().c_str());
  ESP_LOGCONFIG(TAG, "  Target: %s", this->target_.name.c_str());
  ESP_LOGCONFIG(TAG, "  Elster Index: 0x%04x", this->get_elster_index());
  ESP_LOGCONFIG(TAG, "  Elster Type: %d", this->elster_type_);
  LOG_UPDATE_INTERVAL(this);
}

void StiebelEltronCanNumber::control(float value) {
  // Find parent component and send set command
  auto *parent = this->parent_;
  if (!parent) return;

  parent->set_elster_value(value, this->get_elster_index(), this->get_elster_type(), this->target_);

  this->set_timeout(500, [this, parent]() {
    // After setting the value, request it again to ensure it was set correctly
    this->update();
  });
}

void StiebelEltronCanNumber::process_can_message(const CanFrame &frame) {
  if (frame.id != this->target_.canId) return;
  if (frame.get_elster_idx() != this->get_elster_index()) return;

  uint16_t raw_value = frame.get_value();
  if (raw_value == 0x8000) {
    this->publish_state(NAN);
    return;
  }

  float value = decode_elster_data(this->get_elster_type(), (uint16_t)raw_value);
  this->publish_state(value);
}

void StiebelEltronCanNumber::update() {
  this->parent_->request_elster_value(this->get_elster_index(), this->target_);
}

} // namespace stiebel_eltron_can
} // namespace esphome