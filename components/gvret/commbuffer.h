#pragma once

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <esphome/components/canbus/canbus.h>
#include <queue>

namespace esphome {
namespace gvret {

using CAN_FRAME = esphome::canbus::CanFrame;
constexpr uint16_t WIFI_BUFF_SIZE = 2048;

class CommBuffer {
public:
  size_t numAvailableBytes() const { return transmitBuffer.size(); }

  void clearBufferedBytes() { transmitBuffer.clear(); }
  void sendFrameToBuffer(CAN_FRAME &frame, int whichBus);
  void setUseBinarySerialComm(bool arg) { this->useBinarySerialComm = arg; }
#ifdef USE_GVRET_CANFD
  void sendFrameToBuffer(CAN_FRAME_FD &frame, int whichBus);
#endif
  void sendBytesToBuffer(uint8_t *bytes, size_t length);
  void sendByteToBuffer(uint8_t byte);
  void sendString(const std::string &str);
  void sendCharString(const char *str);

  struct Buffer : public std::vector<uint8_t> {
    void push_back32(uint32_t arg) {
      push_back(arg);
      push_back(arg >> 8);
      push_back(arg >> 16);
      push_back(arg >> 24);
    }
    void push_back16(uint16_t arg) {
      push_back(arg);
      push_back(arg >> 8);
    }

    int printf(const char *fmt, ...) {
      va_list args;

      va_start(args, fmt);
      const auto length = snprintf(nullptr, 0, fmt, args);
      va_end(args);
      if (length < 0)
        return length;

      const auto prevSize = this->size();
      this->resize(prevSize + length + 1);

      va_start(args, fmt);
      const auto ret = snprintf(reinterpret_cast<char*>(this->data() + prevSize), length + 1, fmt, args);
      va_end(args);

      return ret;
    }
  };

protected:
  bool useBinarySerialComm = false;
  Buffer transmitBuffer;
};

} // namespace gvret
} // namespace esphome