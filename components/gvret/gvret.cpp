#include "gvret.h"
#include "esphome/components/socket/socket.h"

namespace esphome {
namespace gvret {

static const char *const TAG = "gvret";

void GVERTComponent::setup() {
  this->socket_ =
      socket::socket_ip(SOCK_STREAM, 0); // monitored for incoming connections
  if (this->socket_ == nullptr) {
    ESP_LOGW(TAG, "Could not create socket");
    this->mark_failed();
    return;
  }
  int enable = 1;
  int err =
      this->socket_->setsockopt(SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
  if (err != 0) {
    ESP_LOGW(TAG, "Socket unable to set reuseaddr: errno %d", err);
    // we can still continue
  }
  err = this->socket_->setblocking(false);
  if (err != 0) {
    ESP_LOGW(TAG, "Socket unable to set nonblocking mode: errno %d", err);
    this->mark_failed();
    return;
  }

  struct sockaddr_storage server;

  socklen_t sl = socket::set_sockaddr_any((struct sockaddr *)&server,
                                          sizeof(server), this->port_);
  if (sl == 0) {
    ESP_LOGW(TAG, "Socket unable to set sockaddr: errno %d", errno);
    this->mark_failed();
    return;
  }

  err = this->socket_->bind((struct sockaddr *)&server, sl);
  if (err != 0) {
    ESP_LOGW(TAG, "Socket unable to bind: errno %d", errno);
    this->mark_failed();
    return;
  }

  err = this->socket_->listen(4);
  if (err != 0) {
    ESP_LOGW(TAG, "Socket unable to listen: errno %d", errno);
    this->mark_failed();
    return;
  }
}
void GVERTComponent::loop() {
  while (true) {
    struct sockaddr_storage source_addr;
    socklen_t addr_len = sizeof(source_addr);
    auto sock = socket_->accept((struct sockaddr *)&source_addr, &addr_len);
    if (!sock)
      break;

    ESP_LOGD(TAG, "Accepted %s", sock->getpeername().c_str());

    if (this->active_connection_) {
      ESP_LOGW(TAG, "Already connected. Closing new connection %s",
               sock->getpeername().c_str());
      sock->close();
      continue;
    }

    this->active_connection_ = std::move(sock);
  }

  if (!this->active_connection_) return;

  
}
void GVERTComponent::dump_config() {}
} // namespace gvret
} // namespace esphome