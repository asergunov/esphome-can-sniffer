#pragma once

#include "esphome/core/defines.h"
#include <vector>
#ifdef USE_ESP_IDF

#ifdef USE_ESP32_CAN
#include "esphome/components/esp32_can/esp32_can.h"
#else
#include "esphome/components/canbus/canbus.h"
#endif
#include "esphome/components/sd_mmc_card/sd_card.h"
#include "esphome/core/component.h"

namespace esphome {
namespace can_logger {

using Canbus =
#ifdef USE_ESP32_CAN
    esp32_can::ESP32Can;
#else
    canbus::Canbus;
#endif

enum class Format {
  CRTD,
};

class CanLogger : public Component {
public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  void set_sd_card(sd_mmc_card::SdCard *card) { this->card_ = card; }
  void set_file(std::string path, Format format) {
    this->path_ = path;
    this->format_ = format;
  }
  void add_canbus(Canbus *canbus, int log_id) {
    this->busses_.emplace_back(canbus, log_id);
  }
  float get_setup_priority() const override { return setup_priority::DATA; }

protected:
  sd_mmc_card::File open_();

protected:
  sd_mmc_card::SdCard *card_ = nullptr;
  std::string path_;
  struct Instance {
    Canbus *bus;
    int log_id;
  };
  std::vector<Instance> busses_;
  Format format_ = Format::CRTD;
};

} // namespace can_logger
} // namespace esphome

#endif // USE_ESP_IDF