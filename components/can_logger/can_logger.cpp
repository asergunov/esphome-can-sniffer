#include "can_logger.h"

#include "esphome/components/sd_mmc_card/sd_card.h"
#include "esphome/core/log.h"
#include "sys/_timeval.h"

#include <cstdio>
#include <inttypes.h>
#include <sys/select.h>

#ifdef USE_ESP_IDF

namespace esphome {
namespace can_logger {

static const char *const TAG = "can_logger";

void print_timestamp(sd_mmc_card::File &file) {
  timeval tv;
  if (0 != gettimeofday(&tv, nullptr)) {
    ESP_LOGE(TAG, "Failed to get time");
    return;
  }
  fprintf(file, "%llu.%06lu ", tv.tv_sec, tv.tv_usec);
}

void print_can_command(sd_mmc_card::File &file, int log_id, const char *cmd) {
  fprintf(file, "%i%s", log_id, cmd);
}

void print_can_packet(sd_mmc_card::File &file, bool read, int log_id,
                      uint32_t can_id, bool extended_id, bool rtr,
                      const std::vector<uint8_t> &data) {
  print_can_command(file, log_id, read ? "R" : "T");
  auto &&BIT_COUNT = extended_id ? "29" : "11";
  if (extended_id) {
    fprintf(file, "%s %08" PRIx32 "", BIT_COUNT, can_id);

  } else {
    fprintf(file, "%s %03" PRIx32 "", BIT_COUNT, can_id);
  }
  for (const auto &byte : data) {
    fprintf(file, " %02X", byte);
  }
}

void CanLogger::setup() {
  switch (this->format_) {
  case Format::CRTD:
    for (const auto &bus : this->busses_) {
      bus.bus->add_callback([this, log_id = bus.log_id](
                                uint32_t can_id, bool extended_id, bool rtr,
                                const std::vector<uint8_t> &data) {
        auto &&file = this->open_();
        print_timestamp(file);
        print_can_packet(file, true, log_id, can_id, extended_id, rtr, data);
        fprintf(file, "\n");
      });
      bus.bus->add_transmit_callback([this, log_id = bus.log_id](
                                         uint32_t can_id, bool extended_id,
                                         bool rtr,
                                         const std::vector<uint8_t> &data) {
        auto &&file = this->open_();
        print_timestamp(file);
        print_can_packet(file, false, log_id, can_id, extended_id, rtr, data);
        fprintf(file, "\n");
      });
#ifdef USE_ESP32_CAN
      bus.bus->add_config_callback([this, log_id = bus.log_id](
                                       const twai_general_config_t &g_config,
                                       const twai_timing_config_t &t_config,
                                       const twai_filter_config_t &f_config,
                                       esp_err_t err) {
        auto &&file = this->open_();
        print_timestamp(file);
        print_can_command(file, log_id, "CBC");
        if (t_config.quanta_resolution_hz) {
          fprintf(file, " speed=%" PRIu32 " quanta_resolution_hz=%" PRIu32 " tseg_1=%d tseg_2=%d", 
            t_config.quanta_resolution_hz/(1+t_config.tseg_1 + t_config.tseg_2), t_config.quanta_resolution_hz, t_config.tseg_1, t_config.tseg_2);
        }
        fprintf(file,
                " tx_io=%i rx_io=%i clkout_io=%i bus_off_io=%i "
                "tx_queue_len=%" PRIu32 " rx_queue_len=%" PRIu32
                " alerts_enabled=%" PRIu32 " clkout_divider=%" PRIu32
                " intr_flags=0x%X\n",
                g_config.tx_io, g_config.rx_io, g_config.clkout_io,
                g_config.bus_off_io, g_config.tx_queue_len,
                g_config.rx_queue_len, g_config.alerts_enabled,
                g_config.clkout_divider, g_config.intr_flags);
      });
#endif
    }
    break;
  default:
    ESP_LOGE(TAG, "Format unsupported");
    this->status_set_error();
  }
}
void CanLogger::loop() {}
void CanLogger::dump_config() {}

sd_mmc_card::File CanLogger::open_() {
  return this->card_->open(this->path_.c_str(), "a");
}

} // namespace can_logger
} // namespace esphome
#endif // USE_ESP_IDF