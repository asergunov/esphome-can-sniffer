#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart_component.h"

namespace esphome {
namespace uart_dump {

class UARTDump : public Component {
    
public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  void set_uart(uart::UARTComponent *uart) { this->uart_ = uart; }

protected:
  uart::UARTComponent *uart_ = nullptr;
};

} // namespace uart_dump
} // namespace esphome