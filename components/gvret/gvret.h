#pragma once

#include "esphome/core/defines.h"
#include <vector>

#ifdef USE_ESP32_CAN
#include "esphome/components/esp32_can/esp32_can.h"
#else
#include "esphome/components/canbus/canbus.h"
#endif
#include "esphome/components/sd_mmc_card/sd_card.h"
#include "esphome/components/socket/socket.h"
#include "esphome/core/component.h"

namespace esphome {
namespace gvret {

using Canbus =
#ifdef USE_ESP32_CAN
    esp32_can::ESP32Can;
#else
    canbus::Canbus;
#endif

class GVERTComponent : public Component {
public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  void add_canbus(Canbus *canbus) { this->busses_.emplace_back(canbus); }
  std::vector<Canbus *> busses_;
  uint16_t port_{23};
  std::unique_ptr<socket::Socket> socket_ = nullptr;
  std::unique_ptr<socket::Socket> active_connection_ = nullptr;
  bool connected_ = false;
};

} // namespace gvret
} // namespace esphome