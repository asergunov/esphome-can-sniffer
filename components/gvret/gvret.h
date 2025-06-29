#pragma once

#include "esphome/core/defines.h"

#include "commbuffer.h"
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

enum STATE {
  IDLE,
  GET_COMMAND,
  BUILD_CAN_FRAME,
  TIME_SYNC,
  GET_DIG_INPUTS,
  GET_ANALOG_INPUTS,
  SET_DIG_OUTPUTS,
  SETUP_CANBUS,
  GET_CANBUS_PARAMS,
  GET_DEVICE_INFO,
  SET_SINGLEWIRE_MODE,
  SET_SYSTYPE,
  ECHO_CAN_FRAME,
  SETUP_EXT_BUSES
};

enum GVRET_PROTOCOL {
  PROTO_BUILD_CAN_FRAME = 0,
  PROTO_TIME_SYNC = 1,
  PROTO_DIG_INPUTS = 2,
  PROTO_ANA_INPUTS = 3,
  PROTO_SET_DIG_OUT = 4,
  PROTO_SETUP_CANBUS = 5,
  PROTO_GET_CANBUS_PARAMS = 6,
  PROTO_GET_DEV_INFO = 7,
  PROTO_SET_SW_MODE = 8,
  PROTO_KEEPALIVE = 9,
  PROTO_SET_SYSTYPE = 10,
  PROTO_ECHO_CAN_FRAME = 11,
  PROTO_GET_NUMBUSES = 12,
  PROTO_GET_EXT_BUSES = 13,
  PROTO_SET_EXT_BUSES = 14,
  PROTO_BUILD_FD_FRAME = 20,
  PROTO_SETUP_FD = 21,
  PROTO_GET_FD = 22,
};

using Canbus =
#ifdef USE_ESP32_CAN
    esp32_can::ESP32Can;
#else
    canbus::Canbus;
#endif

class GVERTComponent : public Component, public CommBuffer {
public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  void add_canbus(Canbus *canbus) { this->busses_.emplace_back(canbus); }
protected:
  void toggleRXLED() {}
  void displayFrame(CAN_FRAME &frame, int whichBus);
protected:
  std::vector<Canbus *> busses_;
  uint16_t port_{23};
  std::unique_ptr<socket::Socket> socket_ = nullptr;
  std::unique_ptr<socket::Socket> active_connection_ = nullptr;
  bool connected_ = false;

public:
  void processIncomingByte(uint8_t in_byte);

private:
  CAN_FRAME build_out_frame;
#ifdef USE_GVRET_CANFD
  CAN_FRAME_FD build_out_fd_frame;
#endif
  int out_bus;
  uint8_t buff[20];

  int step = 0;
  STATE state = IDLE;
  uint32_t build_int;
#ifdef USE_LAWICE_MODE
  bool lawicelMode = false;
#endif
  uint8_t checksumCalc(uint8_t *buffer, int length);
  void setOutput(uint8_t which, bool active);
};

} // namespace gvret
} // namespace esphome