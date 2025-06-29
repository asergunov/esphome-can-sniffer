#include "uart_dump.h"

#include <cstdint>

namespace esphome {
namespace uart_dump {

static const char *const TAG = "uart_dump";

void UARTDump::setup() {}

void UARTDump::loop() {
  while (const auto available = this->uart_->available()) {
    uint8_t data[256];
    const auto to_read = std::min<size_t>(available, sizeof(data));
    const auto read = this->uart_->read_array(data, to_read);
    if (!read) {
      ESP_LOGE(TAG, "Failed to read %zu bytes from UART", to_read);
    } else {
      ESP_LOGV(TAG, "Read %zu bytes from UART", to_read);
    }
  }
}

void UARTDump::dump_config() {}

} // namespace uart_dump
} // namespace esphome