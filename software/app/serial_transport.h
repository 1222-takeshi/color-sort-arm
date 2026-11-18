// POSIX serial transport for Sts3215ArmDriver: a tty device at
// 1 Mbps, 8N1, raw mode. Works for the Pico bridge (USB CDC device)
// and for a bus adapter's USB-serial port alike; which device node to
// open is the caller's choice (color_sort --port).
#pragma once

#include <string>

#include "app/sts3215_driver.h"

namespace arm {

class SerialTransport : public ByteTransport {
 public:
  explicit SerialTransport(const std::string& device, int baud = 1000000);
  ~SerialTransport() override;
  SerialTransport(const SerialTransport&) = delete;
  SerialTransport& operator=(const SerialTransport&) = delete;

  void write(const std::vector<uint8_t>& bytes) override;
  std::vector<uint8_t> readSome(int timeout_ms) override;
  void flushInput() override;

 private:
  int fd_ = -1;
};

}  // namespace arm
