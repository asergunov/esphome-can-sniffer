#include "commbuffer.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include <string>

namespace esphome {
namespace gvret {

static const char *const TAG = "gvret.commbuffer";

// a bit faster version that blasts through the copy more efficiently.
void CommBuffer::sendBytesToBuffer(uint8_t *bytes, size_t length) {
  transmitBuffer.insert(transmitBuffer.end(), bytes, bytes + length);
}

void CommBuffer::sendByteToBuffer(uint8_t byt) {
  transmitBuffer.push_back(byt);
}

void CommBuffer::sendString(const std::string &str) {
  sendCharString(str.c_str());
}

void CommBuffer::sendCharString(const char *str) {
  const char *p = str;
  int i = 0;
  while (*p) {
    sendByteToBuffer(*p++);
    i++;
  }
  ESP_LOGV(TAG, "Queued %i bytes", i);
}

void CommBuffer::sendFrameToBuffer(CAN_FRAME &frame, int whichBus) {
  uint8_t temp;
  size_t writtenBytes;
  if (this->useBinarySerialComm) {
    if (frame.use_extended_id)
      frame.can_id |= 1 << 31;
    transmitBuffer.push_back(0xF1);
    transmitBuffer.push_back(0); // 0 = canbus frame sending
    uint32_t now = micros();
    transmitBuffer.push_back32(now);
    transmitBuffer.push_back32(frame.can_id);
    transmitBuffer.push_back(frame.can_data_length_code +
                             (uint8_t)(whichBus << 4));
    for (int c = 0; c < frame.can_data_length_code; c++) {
      transmitBuffer.push_back(frame.data[c]);
    }
    // temp = checksumCalc(buff, 11 + frame.length);
    temp = 0;
    transmitBuffer.push_back(temp);
    // Serial.write(buff, 12 + frame.length);
  } else {
    transmitBuffer.printf("%d - %x", micros(), frame.can_id);
    if (frame.use_extended_id)
      transmitBuffer.printf(" X ");
    else
      transmitBuffer.printf(" S ");
    transmitBuffer.printf("%i %i", whichBus, frame.can_data_length_code);
    for (int c = 0; c < frame.can_data_length_code; c++) {
      transmitBuffer.printf(" %x", frame.data[c]);
    }
    transmitBuffer.printf("\r\n");
  }
}
#ifdef USE_GVRET_CANFD

void CommBuffer::sendFrameToBuffer(CAN_FRAME_FD &frame, int whichBus) {
  uint8_t temp;
  size_t writtenBytes;
  if (settings.useBinarySerialComm) {
    if (frame.extended)
      frame.id |= 1 << 31;
    transmitBuffer.push_back(0xF1);
    transmitBuffer.push_back(PROTO_BUILD_FD_FRAME);
    uint32_t now = micros();
    transmitBuffer.push_back32(now);
    transmitBuffer.push_back32(frame.id);
    transmitBuffer.push_back(frame.length);
    transmitBuffer.push_back((uint8_t)(whichBus));
    for (int c = 0; c < frame.length; c++) {
      transmitBuffer.push_back(frame.data.uint8[c]);
    }
    // temp = checksumCalc(buff, 11 + frame.length);
    temp = 0;
    transmitBuffer.push_back(temp);
    // Serial.write(buff, 12 + frame.length);
  } else {
    transmitBuffer.printf("%d - %x", micros(), frame.id);
    if (frame.extended)
      transmitBuffer.printf(" X ");
    else
      transmitBuffer.printf(" S ");
    transmitBuffer.printf("%i %i", whichBus, frame.length);
    for (int c = 0; c < frame.length; c++) {
      transmitBuffer.printf(" %x", frame.data.uint8[c]);
    }
    transmitBuffer.printf("\r\n");
  }
}
#endif
} // namespace gvret
} // namespace esphome