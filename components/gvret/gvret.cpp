#define protected public

#include "gvret.h"
#include "esphome/components/socket/socket.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include <cstdint>
#include <sys/types.h>

namespace esphome {
namespace gvret {

static const char *const TAG = "gvret";
constexpr uint16_t CFG_BUILD_NUM = 618;

/// Is the given return value (from write syscalls) a wouldblock error?
bool is_would_block(ssize_t ret) {
  if (ret == -1) {
    return errno == EWOULDBLOCK || errno == EAGAIN;
  }
  return ret == 0;
}

void GVERTComponent::setup() {

  int index = 0; 
  for(auto& bus : this->busses_) {
    bus->add_callback([this, index](uint32_t can_id, bool extended_id, bool rtr, const std::vector<uint8_t> &data){
      CAN_FRAME frame;
      frame.can_id = can_id;
      frame.use_extended_id = extended_id;
      frame.remote_transmission_request = rtr;
      frame.can_data_length_code = std::min<uint8_t>(data.size(), canbus::CAN_MAX_DATA_LENGTH);
      std::copy_n(data.begin(), frame.can_data_length_code, frame.data);
      this->displayFrame(frame, index);
    });
    index++;
  }


  ESP_LOGD(TAG, "Creating a socket");
  this->socket_ =
      socket::socket_ip(SOCK_STREAM, 0); // monitored for incoming connections
  if (this->socket_ == nullptr) {
    ESP_LOGW(TAG, "Could not create socket");
    this->mark_failed();
    return;
  }
  ESP_LOGD(TAG, "Enabling a socket");
  int enable = 1;
  int err =
      this->socket_->setsockopt(SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
  if (err != 0) {
    ESP_LOGW(TAG, "Socket unable to set reuseaddr: errno %d", err);
    // we can still continue
  }
  ESP_LOGD(TAG, "Set non-blocking");
  err = this->socket_->setblocking(false);
  if (err != 0) {
    ESP_LOGW(TAG, "Socket unable to set nonblocking mode: errno %d", err);
    this->mark_failed();
    return;
  }

  struct sockaddr_storage server;
  ESP_LOGD(TAG, "Set socket address");
  socklen_t sl = socket::set_sockaddr_any((struct sockaddr *)&server,
                                          sizeof(server), this->port_);
  if (sl == 0) {
    ESP_LOGW(TAG, "Socket unable to set sockaddr: errno %d", errno);
    this->mark_failed();
    return;
  }

  ESP_LOGD(TAG, "Binding socket");
  err = this->socket_->bind((struct sockaddr *)&server, sl);
  if (err != 0) {
    ESP_LOGW(TAG, "Socket unable to bind: errno %d", errno);
    this->mark_failed();
    return;
  }

  ESP_LOGD(TAG, "Listening socket");
  err = this->socket_->listen(4);
  if (err != 0) {
    ESP_LOGW(TAG, "Socket unable to listen: errno %d", errno);
    this->mark_failed();
    return;
  }
}

void GVERTComponent::loop() {
  for (;;) {
    struct sockaddr_storage source_addr;
    socklen_t addr_len = sizeof(source_addr);
    auto sock = socket_->accept((struct sockaddr *)&source_addr, &addr_len);
    if (!sock)
      break;

    ESP_LOGD(TAG, "Setting non-blocking");
    sock->setblocking(false);
    ESP_LOGD(TAG, "Accepted %s", sock->getpeername().c_str());

    if (this->active_connection_) {
      ESP_LOGW(TAG, "Already connected. Closing new connection %s",
               sock->getpeername().c_str());
      sock->close();
      continue;
    }

    this->active_connection_ = std::move(sock);
  }

  if (!this->active_connection_) {
    ESP_LOGVV(TAG, "No active connection. Return.");
    return;
  }

  while (!transmitBuffer.empty()) {
    ESP_LOGVV(TAG, "Transmitting %zu bytes to socket", transmitBuffer.size());
    ssize_t sent = this->active_connection_->write(transmitBuffer.data(),
                                                   transmitBuffer.size());
    if (is_would_block(sent)) {
      break;
    } else if (sent == -1) {
      ESP_LOGW(TAG, "Socket write failed with errno %d", errno);
      this->state = IDLE; // TODO: make sure it's the one we like
    }
    transmitBuffer.erase(transmitBuffer.begin(), transmitBuffer.begin() + sent);
  }

  if (!this->active_connection_->ready())
    return;

  ESP_LOGVV(TAG,"Start reading from socket");
  for (;this->active_connection_->ready();) {
    ESP_LOGVV(TAG, "Trying to read byte from socket");
    uint8_t incomingByte;
    // Reading one byte at a time is fastest in practice for ESP32 when
    // there is no data on the wire (which is the common case).
    // This results in faster failure detection compared to
    // attempting to read multiple bytes at once.
    ssize_t received = this->active_connection_->read(&incomingByte, 1);
    if (received == -1) {
      if (errno == ENOTCONN) {
        ESP_LOGI(TAG, "Connection closed");
        this->active_connection_ = nullptr;
        return;
      } else if (errno == EWOULDBLOCK || errno == EAGAIN) {
        ESP_LOGVV(TAG, "EWOULDBLOCK or EAGAIN");
        return;
      }
      ESP_LOGE(TAG, "Socket read failed with errno %d", errno);
      return;
    } else if (received == 0) {
      state = IDLE;
      ESP_LOGI(TAG, "Connection closed");
      return;
    }
    processIncomingByte(incomingByte);
  }

  ESP_LOGVV(TAG,"End reading from socket");
}

void GVERTComponent::dump_config() {}

void GVERTComponent::processIncomingByte(uint8_t in_byte) {
  ESP_LOGVV(TAG, "Processing incoming byte 0x%02X in state %d", in_byte, state);
  uint32_t busSpeed = 0;
  uint32_t now = micros();

  switch (state) {
  default:
    ESP_LOGV(TAG, "Unknown state %d", state);
    break;
  case IDLE:
    ESP_LOGVV(TAG, "In state IDLE");
    if (in_byte == 0xF1) {
      ESP_LOGVV(TAG, "Next byte is command");
      state = GET_COMMAND;
    } else if (in_byte == 0xE7) {
      ESP_LOGV(TAG, "Use binary serial comm");
      setUseBinarySerialComm(true);
#ifdef USE_LAWICE_MODE
      lawicelMode = false;
#endif
      // setPromiscuousMode(); //going into binary comm will set promisc. mode
      // too.
    } else {
      // TODO: figure out what should be here
      // console.rcvCharacter((uint8_t)in_byte);
    }
    break;
  case GET_COMMAND:
    ESP_LOGVV(TAG, "In state GET_COMMAND");
    switch (in_byte) {
    default:
      ESP_LOGV(TAG, "Unknown command 0x%02X", in_byte);
      break;
    case PROTO_BUILD_CAN_FRAME:
      ESP_LOGV(TAG, "Command is PROTO_BUILD_CAN_FRAME");
      state = BUILD_CAN_FRAME;
      buff[0] = 0xF1;
      step = 0;
      break;
    case PROTO_TIME_SYNC:
      ESP_LOGV(TAG, "Command is PROTO_TIME_SYNC");
      state = TIME_SYNC;
      step = 0;
      transmitBuffer.push_back(0xF1);
      transmitBuffer.push_back(1); // time sync
      transmitBuffer.push_back32(now);
      break;
    case PROTO_DIG_INPUTS:
      ESP_LOGV(TAG, "Command is PROTO_DIG_INPUTS");
      // immediately return the data for digital inputs
      transmitBuffer.push_back(0xF1);
      transmitBuffer.push_back(2); // digital inputs
      transmitBuffer.push_back(
          0
          // getDigital(0) + (getDigital(1) << 1) + (getDigital(2) << 2)
          // + (getDigital(3) << 3) + (getDigital(4) << 4) +
          // (getDigital(5) << 5);
      );
      transmitBuffer.push_back(checksumCalc(buff, 2));
      state = IDLE;
      break;
    case PROTO_ANA_INPUTS:
      ESP_LOGV(TAG, "Command is PROTO_ANA_INPUTS");
      // immediately return data on analog inputs
      transmitBuffer.push_back(0xF1);
      transmitBuffer.push_back(3);
      transmitBuffer.push_back16(0); // getAnalog(0);  // Analogue input 1
      transmitBuffer.push_back16(0); // getAnalog(1);  // Analogue input 2
      transmitBuffer.push_back16(0); // getAnalog(2);  // Analogue input 3
      transmitBuffer.push_back16(0); // getAnalog(3);  // Analogue input 4
      transmitBuffer.push_back16(0); // getAnalog(4);  // Analogue input 5
      transmitBuffer.push_back16(0); // getAnalog(5);  // Analogue input 6
      transmitBuffer.push_back16(0); // getAnalog(6);  // Vehicle Volts
      transmitBuffer.push_back(checksumCalc(buff, 9));
      state = IDLE;
      break;
    case PROTO_SET_DIG_OUT:
      ESP_LOGV(TAG, "Command is SET_DIG_OUTPUTS");
      state = SET_DIG_OUTPUTS;
      buff[0] = 0xF1;
      break;
    case PROTO_SETUP_CANBUS:
      ESP_LOGV(TAG, "Command is PROTO_SETUP_CANBUS");
      state = SETUP_CANBUS;
      step = 0;
      buff[0] = 0xF1;
      break;
    case PROTO_GET_CANBUS_PARAMS:
      ESP_LOGV(TAG, "Command is PROTO_GET_CANBUS_PARAMS");
      // immediately return data on canbus params
      transmitBuffer.push_back(0xF1);
      transmitBuffer.push_back(6);
      if (this->busses_.size() > 0) {
        transmitBuffer.push_back(
#ifdef USE_ESP32_CAN
            (uint8_t(this->busses_[0]->get_enabled()) ? 1 : 0) +
            (uint8_t(busses_[0]->get_tx_mode() == esp32_can::TXMode::LISTEN_ONLY
                         ? 1
                         : 0)
             << 4)
#else
            1 // enabled
#endif
        );
        transmitBuffer.push_back32(busses_[0]->get_bits_per_second());
      } else {
        transmitBuffer.push_back(0);
        transmitBuffer.push_back32(0);
      }

      if (this->busses_.size() > 1) {
        transmitBuffer.push_back(
#ifdef USE_ESP32_CAN
            uint8_t(this->busses_[1]->get_enabled() ? 1 : 0) +
            (uint8_t(busses_[1]->get_tx_mode() == esp32_can::TXMode::LISTEN_ONLY
                         ? 1
                         : 0)
             << 4)
#else
            1
#endif
        );
        transmitBuffer.push_back32(busses_[1]->get_bits_per_second());
      } else {
        transmitBuffer.push_back(0);
        transmitBuffer.push_back32(0);
      }
      state = IDLE;
      break;
    case PROTO_GET_DEV_INFO:
      ESP_LOGV(TAG, "Command is PROTO_GET_DEV_INFO");
      // immediately return device information
      transmitBuffer.push_back(0xF1);
      transmitBuffer.push_back(7);
      transmitBuffer.push_back16(CFG_BUILD_NUM);
      transmitBuffer.push_back(0x20);
      transmitBuffer.push_back(0);
      transmitBuffer.push_back(0);
      transmitBuffer.push_back(
          0); // was single wire mode. Should be rethought for this board.
      state = IDLE;
      break;
    case PROTO_SET_SW_MODE:
      ESP_LOGV(TAG, "Command is PROTO_SET_SW_MODE");
      buff[0] = 0xF1;
      state = SET_SINGLEWIRE_MODE;
      step = 0;
      break;
    case PROTO_KEEPALIVE:
      ESP_LOGVV(TAG, "Command is PROTO_KEEPALIVE");
      transmitBuffer.push_back(0xF1);
      transmitBuffer.push_back(0x09);
      transmitBuffer.push_back(0xDE);
      transmitBuffer.push_back(0xAD);
      state = IDLE;
      break;
    case PROTO_SET_SYSTYPE:
      ESP_LOGV(TAG, "Command is PROTO_SET_SYSTYPE");
      buff[0] = 0xF1;
      state = SET_SYSTYPE;
      step = 0;
      break;
    case PROTO_ECHO_CAN_FRAME:
      ESP_LOGV(TAG, "Command is PROTO_ECHO_CAN_FRAME");
      state = ECHO_CAN_FRAME;
      buff[0] = 0xF1;
      step = 0;
      break;
    case PROTO_GET_NUMBUSES:
      ESP_LOGV(TAG, "Command is PROTO_GET_NUMBUSES");
      transmitBuffer.push_back(0xF1);
      transmitBuffer.push_back(12);
      transmitBuffer.push_back(this->busses_.size());
      state = IDLE;
      break;
    case PROTO_GET_EXT_BUSES:
      ESP_LOGV(TAG, "Command is PROTO_GET_EXT_BUSES");
      transmitBuffer.push_back(0xF1);
      transmitBuffer.push_back(13);
      for (int u = 2; u < 17; u++)
        transmitBuffer.push_back(0);
      step = 0;
      state = IDLE;
      break;
    case PROTO_SET_EXT_BUSES:
      ESP_LOGV(TAG, "Command is PROTO_SET_EXT_BUSES");
      state = SETUP_EXT_BUSES;
      step = 0;
      buff[0] = 0xF1;
      break;
    }
    break;
  case BUILD_CAN_FRAME:
    ESP_LOGVV(TAG, "In state BUILD_CAN_FRAME");
    buff[1 + step] = in_byte;
    switch (step) {
    case 0:
      build_out_frame.can_id = in_byte;
      break;
    case 1:
      build_out_frame.can_id |= in_byte << 8;
      break;
    case 2:
      build_out_frame.can_id |= in_byte << 16;
      break;
    case 3:
      build_out_frame.can_id |= in_byte << 24;
      if (build_out_frame.can_id & 1 << 31) {
        build_out_frame.can_id &= 0x7FFFFFFF;
        build_out_frame.use_extended_id = true;
      } else
        build_out_frame.use_extended_id = false;
      break;
    case 4:
      out_bus = in_byte & 3;
      break;
    case 5:
      build_out_frame.can_data_length_code = in_byte & 0xF;
      if (build_out_frame.can_data_length_code > 8) {
        build_out_frame.can_data_length_code = 8;
      }
      break;
    default:
      if (step < build_out_frame.can_data_length_code + 6) {
        build_out_frame.data[step - 6] = in_byte;
      } else {
        state = IDLE;
        // this would be the checksum byte. Compute and compare.
        // temp8 = checksumCalc(buff, step);
        build_out_frame.remote_transmission_request = 0;
        if (out_bus < this->busses_.size()) {
          ESP_LOGD(TAG,"Sending message to CAN %d", out_bus);
          this->busses_[out_bus]->send_message(&build_out_frame);
        } else {
          ESP_LOGW(TAG,"Don't have CAN %d to send message", out_bus);
        }
      }
      break;
    }
    step++;
    break;
  case TIME_SYNC:
    ESP_LOGVV(TAG, "In state TIME_SYNC");
    state = IDLE;
    break;
  case GET_DIG_INPUTS:
    ESP_LOGVV(TAG, "In state GET_DIG_INPUTS");
    // nothing to do
    break;
  case GET_ANALOG_INPUTS:
    ESP_LOGVV(TAG, "In state GET_ANALOG_INPUTS");
    // nothing to do
    break;
  case SET_DIG_OUTPUTS: // todo: validate the XOR byte
    ESP_LOGVV(TAG, "In state SET_DIG_OUTPUTS");
    buff[1] = in_byte;
    // temp8 = checksumCalc(buff, 2);
    for (uint8_t c = 0; c < 8; c++) {
      setOutput(c, in_byte & (1 << c));
    }
    state = IDLE;
    break;
  case SETUP_CANBUS: // todo: validate checksum
    ESP_LOGVV(TAG, "In state SETUP_CANBUS");
    switch (step) {
    case 0:
      build_int = in_byte;
      break;
    case 1:
      build_int |= in_byte << 8;
      break;
    case 2:
      build_int |= in_byte << 16;
      break;
    case 3:
      build_int |= in_byte << 24;
      busSpeed = build_int & 0xFFFFF;
      if (busSpeed > 1000000)
        busSpeed = 1000000;

      if (this->busses_.size() < 1)
        break;

      if (build_int > 0) {
#ifdef USE_ESP32_CAN
        if (build_int & 0x80000000ul) // signals that enabled and listen only
                                      // status are also being passed
        {
          this->busses_[0]->set_enabled(build_int & 0x40000000ul);
          this->busses_[0]->set_tx_mode(
              build_int & 0x20000000ul
                  ? esp32_can::TXMode::LISTEN_ONLY
                  : esp32_can::TXMode::NO_ACK); // TODO: make sure NO_ACK is
                                                // what wre like to have here
        } else {
          // if not using extended status mode then just default to enabling -
          // this was old behavior
          this->busses_[0]->set_enabled(true);
        }
#endif
        // CAN0.set_baudrate(build_int);
        this->busses_[0]->set_bits_per_second(busSpeed);

      } else { // disable first canbus
#ifdef USE_ESP32_CAN
        busses_[0]->set_enabled(false);
#endif
      }
#ifdef USE_ESP32_CAN
      if (busses_[0]->get_enabled())
        busses_[0]->start();
      else
        busses_[0]->stop();
#endif

      break;
    case 4:
      build_int = in_byte;
      break;
    case 5:
      build_int |= in_byte << 8;
      break;
    case 6:
      build_int |= in_byte << 16;
      break;
    case 7:
      build_int |= in_byte << 24;
      busSpeed = build_int & 0xFFFFF;
      if (busSpeed > 1000000)
        busSpeed = 1000000;

      if (this->busses_.size() < 2)
        break;

      if (build_int > 0) {
#ifdef USE_ESP32_CAN
        if (build_int & 0x80000000ul) // signals that enabled and listen only
                                      // status are also being passed
        {
          busses_[1]->set_enabled(build_int & 0x40000000ul);
          this->busses_[0]->set_tx_mode(
              build_int & 0x20000000ul
                  ? esp32_can::TXMode::LISTEN_ONLY
                  : esp32_can::TXMode::NO_ACK); // TODO: make sure NO_ACK is
                                                // what wre like to have here
        } else {
          // if not using extended status mode then just default to enabling -
          // this was old behavior
          busses_[1]->set_enabled(false);
        }
#endif
        // CAN1.set_baudrate(build_int);
        this->busses_[1]->set_bits_per_second(busSpeed);
      }

      state = IDLE;
      // now, write out the new canbus settings to EEPROM
      // EEPROM.writeBytes(0, &settings, sizeof(settings));
      // EEPROM.commit();
      // setPromiscuousMode();
      break;
    }
    step++;
    break;
  case GET_CANBUS_PARAMS:
    ESP_LOGVV(TAG, "In state GET_CANBUS_PARAMS");
    // nothing to do
    break;
  case GET_DEVICE_INFO:
    ESP_LOGVV(TAG, "In state GET_DEVICE_INFO");
    // nothing to do
    break;
  case SET_SINGLEWIRE_MODE:
    ESP_LOGVV(TAG, "In state SET_SINGLEWIRE_MODE");
    if (in_byte == 0x10) {
    } else {
    }
    // EEPROM.writeBytes(0, &settings, sizeof(settings));
    // EEPROM.commit();
    state = IDLE;
    break;
  case SET_SYSTYPE:
    ESP_LOGVV(TAG, "In state SET_SYSTYPE");
    ESP_LOGW(TAG, "SET_SYSTYPE is not supported");
    // settings.systemType = in_byte;
    // // EEPROM.writeBytes(0, &settings, sizeof(settings));
    // // EEPROM.commit();
    // // loadSettings();
    // state = IDLE;
    break;
  case ECHO_CAN_FRAME:
    ESP_LOGVV(TAG, "In state ECHO_CAN_FRAME");
    buff[1 + step] = in_byte;
    switch (step) {
    case 0:
      build_out_frame.can_id = in_byte;
      break;
    case 1:
      build_out_frame.can_id |= in_byte << 8;
      break;
    case 2:
      build_out_frame.can_id |= in_byte << 16;
      break;
    case 3:
      build_out_frame.can_id |= in_byte << 24;
      if (build_out_frame.can_id & 1 << 31) {
        build_out_frame.can_id &= 0x7FFFFFFF;
        build_out_frame.use_extended_id = true;
      } else
        build_out_frame.use_extended_id = false;
      break;
    case 4:
      out_bus = in_byte & 1;
      break;
    case 5:
      build_out_frame.can_data_length_code = in_byte & 0xF;
      if (build_out_frame.can_data_length_code > 8)
        build_out_frame.can_data_length_code = 8;
      break;
    default:
      if (step < build_out_frame.can_data_length_code + 6) {
        build_out_frame.data[step - 6] = in_byte;
      } else {
        state = IDLE;
        // this would be the checksum byte. Compute and compare.
        // temp8 = checksumCalc(buff, step);
        // if (temp8 == in_byte)
        //{
        toggleRXLED();
        // if(isConnected) {
        displayFrame(build_out_frame, 0);
        //}
        //}
      }
      break;
    }
    step++;
    break;
  case SETUP_EXT_BUSES: // setup enable/listenonly/speed for SWCAN, Enable/Speed
                        // for LIN1, LIN2

    ESP_LOGVV(TAG, "In state SETUP_EXT_BUSES");
    switch (step) {
    case 0:
      build_int = in_byte;
      break;
    case 1:
      build_int |= in_byte << 8;
      break;
    case 2:
      build_int |= in_byte << 16;
      break;
    case 3:
      build_int |= in_byte << 24;
      break;
    case 4:
      build_int = in_byte;
      break;
    case 5:
      build_int |= in_byte << 8;
      break;
    case 6:
      build_int |= in_byte << 16;
      break;
    case 7:
      build_int |= in_byte << 24;
      break;
    case 8:
      build_int = in_byte;
      break;
    case 9:
      build_int |= in_byte << 8;
      break;
    case 10:
      build_int |= in_byte << 16;
      break;
    case 11:
      build_int |= in_byte << 24;
      state = IDLE;
      // now, write out the new canbus settings to EEPROM
      // EEPROM.writeBytes(0, &settings, sizeof(settings));
      // EEPROM.commit();
      // setPromiscuousMode();
      break;
    }
    step++;
    break;
  }
}

// Get the value of XOR'ing all the bytes together. This creates a reasonable
// checksum that can be used to make sure nothing too stupid has happened on the
// comm.
uint8_t GVERTComponent::checksumCalc(uint8_t *buffer, int length) {
  uint8_t value = 0;
  for (int c = 0; c < length; c++) {
    value ^= buffer[c];
  }
  return value;
}

void GVERTComponent::setOutput(uint8_t which, bool active) {
  // TODO: find out if we need it
  // if ((which >= NUM_OUTPUT) || (out[which] == 255)) {
  //   return;
  // }

  // (which <= 2) ? digitalWrite(out[which], active ? HIGH : LOW)
  //              : digitalWrite(out[which], active ? LOW : HIGH);
}

void GVERTComponent::displayFrame(CAN_FRAME &frame, int whichBus) {
#ifdef USE_LAWICE_MODE
  if (settings.enableLawicel && lawicelMode) {
    lawicel.sendFrameToBuffer(frame, whichBus);
  } else
#endif
  {
    sendFrameToBuffer(frame, whichBus);
  }
}

} // namespace gvret
} // namespace esphome