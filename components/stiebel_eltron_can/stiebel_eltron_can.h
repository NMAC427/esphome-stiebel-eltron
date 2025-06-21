#pragma once

#include "esphome.h"
#include "esphome/core/component.h"
#include "esphome/components/canbus/canbus.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/number/number.h"
#include <vector>
#include <map>
#include <list>

#include "type.h"
#include "can_member.h"

#if __has_include("esphome/components/light/light_state.h")
  #include "esphome/components/light/light_state.h"
  #define _SE_USE_STATUS_LED
#endif

namespace esphome {
namespace stiebel_eltron_can {

class StiebelEltronCanSensor;
class StiebelEltronCanNumber;

class StiebelEltronCanComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  void set_canbus(canbus::Canbus *canbus) { this->canbus_ = canbus; }
#ifdef _SE_USE_STATUS_LED
  void set_status_led(light::LightState *light) { this->status_led_ = light; }
#endif
  void register_sensor(StiebelEltronCanSensor *sensor);
  void register_number(StiebelEltronCanNumber *number);

  void request_elster_value(uint16_t elster_index, CanMember target, bool immediately = false);
  void set_elster_value(float value, uint16_t elster_index, Type elster_type, CanMember target);

 protected:
  canbus::Canbus *canbus_;
  std::vector<StiebelEltronCanSensor*> sensors_;
  std::vector<StiebelEltronCanNumber*> numbers_;

#ifdef _SE_USE_STATUS_LED
  light::LightState *status_led_;
#endif

  CanMember sender_ = can_members::ESPClient;
  std::list<CanFrame> send_queue;

  void process_can_message(uint32_t can_id, const std::vector<uint8_t> &data);
  void blink_status_led();
};

uint16_t encode_elster_data(Type type, float value);
float decode_elster_data(uint16_t type, uint16_t data);

class StiebelEltronCanSensor : public sensor::Sensor, public PollingComponent {
 public:
  void setup() override;
  void dump_config() override;

  void auto_accuracy_decimals();

  void set_parent(StiebelEltronCanComponent *parent) { this->parent_ = parent; }

  void set_target(CanMember target) { this->target_ = target; }
  CanMember get_target() const { return this->target_; }
  void set_elster_index(uint16_t index) { this->elster_index_ = index; }
  uint16_t get_elster_index() const { return this->elster_index_; }
  void set_elster_type(Type type) { this->elster_type_ = type; }
  Type get_elster_type() const { return this->elster_type_; }

  void process_can_message(const CanFrame &frame);

 protected:
  CanMember target_;
  uint16_t elster_index_;
  Type elster_type_;

  StiebelEltronCanComponent *parent_;

  std::tuple<float, float, float> combined_value_;
  float combine_value() const;

  void update() override;
};

class StiebelEltronCanNumber : public number::Number, public PollingComponent {
 public:
  void setup() override;
  void dump_config() override;

  void set_parent(StiebelEltronCanComponent *parent) { this->parent_ = parent; }
  void set_target(CanMember target) { this->target_ = target; }
  CanMember get_target() const { return this->target_; }
  void set_elster_index(uint16_t index) { this->elster_index_ = index; }
  uint16_t get_elster_index() const { return this->elster_index_; }
  void set_elster_type(Type type) { this->elster_type_ = type; }
  Type get_elster_type() const { return this->elster_type_; }

  void process_can_message(const CanFrame &frame);
  void control(float value) override;

 protected:
  CanMember target_;
  uint16_t elster_index_;
  Type elster_type_;

  StiebelEltronCanComponent *parent_;

  void update() override;
};

} // namespace stiebel_eltron_can
} // namespace esphome