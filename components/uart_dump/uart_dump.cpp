#include "uart_dump.h"

#include <cstdint>

namespace esphome {
namespace uart_dump {
  void UARTDump::setup(){  }
  void UARTDump::loop(){
    while(this->uart_->available()) {
        uint8_t data[256];
        this->uart_->read_array(data, sizeof(data));
    }
  }
  void UARTDump::dump_config(){}
}
}