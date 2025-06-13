#pragma once

#include "esphome/core/defines.h"
#include <cstdint>
#include <vector>
#ifdef USE_ESP_IDF

#ifdef USE_ESP32_CAN
#include "esphome/components/esp32_can/esp32_can.h"
#else
#include "esphome/components/canbus/canbus.h"
#endif
#include "esphome/components/sd_mmc_card/sd_card.h"
#include "esphome/components/uart/uart_component.h"
#include "esphome/core/component.h"

namespace esphome {
namespace sdcard_logger {

using Canbus =
#ifdef USE_ESP32_CAN
    esp32_can::ESP32Can;
#else
    canbus::Canbus;
#endif

enum class Format {
  CRTD,
};

class SDCardLogger : public Component {
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
    this->can_busses_.emplace_back(canbus, log_id);
  }
  void add_uart(uart::UARTComponent *uart, int log_id) {}
  float get_setup_priority() const override { return setup_priority::DATA; }

protected:
  sd_mmc_card::File open_();

protected:
  sd_mmc_card::SdCard *card_ = nullptr;
  std::string path_;

  template <typename T> struct Instance {
    T *bus;
    int log_id;
  };

  template <typename T, typename Item>
  struct ThrottlingInstance : public Instance<T> {
    std::vector<Item> buffer;
  };

  std::vector<Instance<Canbus>> can_busses_;
#ifdef USE_UART_DEBUGGER
  struct UartItem {
    uart::UARTDirection direction;
    std::vector<uint8_t> bytes;

    UartItem(uart::UARTDirection direction) : direction(direction) {}
  };
  std::vector<ThrottlingInstance<uart::UARTComponent, UartItem>> uart_busses_;
#endif
  Format format_ = Format::CRTD;
};

} // namespace sdcard_logger
} // namespace esphome

#endif // USE_ESP_IDF